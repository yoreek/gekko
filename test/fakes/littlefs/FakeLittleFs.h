#pragma once

// Minimal in-memory hierarchical filesystem test double. Matches the exact method surface
// LittleFsBlobStoreCore<Fs, FileT> calls on the real fs::LittleFSFS/fs::File so the SAME
// algorithm (not a reimplementation of it) can be instantiated and exercised in native unit
// tests - see test/test_core/test_littlefs_blob_store.cpp.

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ewfm {
namespace test {

struct FakeFsNode {
    bool isDir{false};
    std::vector<uint8_t> data;
    std::map<std::string, std::shared_ptr<FakeFsNode>> children;
};

class FakeFile {
public:
    FakeFile() = default;
    FakeFile(std::shared_ptr<FakeFsNode> node, std::string path) : node_(std::move(node)), path_(std::move(path)) {}

    explicit operator bool() const {
        return node_ != nullptr;
    }

    bool isDirectory() const {
        return node_ && node_->isDir;
    }

    const char* path() const {
        return path_.c_str();
    }

    size_t size() const {
        return node_ ? node_->data.size() : 0U;
    }

    size_t write(const uint8_t* data, const size_t len) {
        if (!node_ || node_->isDir) {
            return 0U;
        }
        node_->data.insert(node_->data.end(), data, data + len);
        return len;
    }

    size_t read(uint8_t* out, const size_t len) {
        if (!node_ || node_->isDir) {
            return 0U;
        }
        const size_t avail = node_->data.size() > pos_ ? node_->data.size() - pos_ : 0U;
        const size_t n = len < avail ? len : avail;
        if (n > 0U) {
            std::memcpy(out, node_->data.data() + pos_, n);
        }
        pos_ += n;
        return n;
    }

    void close() {}

    // Iterates this directory's children one at a time across repeated calls, matching
    // fs::File::openNextFile()'s stateful-cursor behavior. Returns a falsy FakeFile once
    // exhausted.
    FakeFile openNextFile() {
        if (!node_ || !node_->isDir || childCursor_ >= node_->children.size()) {
            return FakeFile();
        }
        auto it = node_->children.begin();
        std::advance(it, static_cast<long>(childCursor_));
        ++childCursor_;
        const std::string childPath = (path_ == "/" ? std::string() : path_) + "/" + it->first;
        return FakeFile(it->second, childPath);
    }

private:
    std::shared_ptr<FakeFsNode> node_;
    std::string path_;
    size_t pos_{0};
    size_t childCursor_{0};
};

class FakeLittleFs {
public:
    using File = FakeFile;

    FakeLittleFs() : root_(std::make_shared<FakeFsNode>()) {
        root_->isDir = true;
    }

    // Test-only knob: simulate free-space pressure to exercise LittleFsBlobStoreCore's
    // free-space guard without actually writing capacity-bytes of data.
    void setCapacity(const size_t total, const size_t used) {
        totalBytes_ = total;
        usedBytes_ = used;
    }

    size_t totalBytes() const {
        return totalBytes_;
    }
    size_t usedBytes() const {
        return usedBytes_;
    }

    bool exists(const char* path) const {
        return find(path) != nullptr;
    }

    bool mkdir(const char* path) {
        return findOrCreate(path, /*asDir=*/true) != nullptr;
    }

    bool rmdir(const char* path) {
        const auto [parent, name] = splitParent(path);
        if (!parent || name.empty()) {
            return false;
        }
        auto it = parent->children.find(name);
        if (it == parent->children.end() || !it->second->isDir || !it->second->children.empty()) {
            return false;
        }
        parent->children.erase(it);
        return true;
    }

    bool remove(const char* path) {
        const auto [parent, name] = splitParent(path);
        if (!parent || name.empty()) {
            return false;
        }
        auto it = parent->children.find(name);
        if (it == parent->children.end() || it->second->isDir) {
            return false;
        }
        parent->children.erase(it);
        return true;
    }

    bool rename(const char* pathFrom, const char* pathTo) {
        const auto [fromParent, fromName] = splitParent(pathFrom);
        if (!fromParent) {
            return false;
        }
        auto it = fromParent->children.find(fromName);
        if (it == fromParent->children.end()) {
            return false;
        }
        const std::shared_ptr<FakeFsNode> node = it->second;

        const auto [toParent, toName] = splitParent(pathTo);
        if (!toParent || toName.empty()) {
            return false;
        }
        fromParent->children.erase(it);
        toParent->children[toName] = node; // overwrites any existing node at the destination
        return true;
    }

    // Mirrors fs::FS::open(path, mode, create): mode[0] != 'r' with create=true walks and
    // creates every missing intermediate directory (matching the real vfs_api.cpp behavior that
    // LittleFsBlobStoreCore::beginPut() depends on for arbitrarily nested keys).
    FakeFile open(const char* path, const char* mode = "r", const bool create = false) {
        const bool writing = mode != nullptr && mode[0] != 'r';
        std::shared_ptr<FakeFsNode> node = find(path);
        if (!node) {
            if (!writing) {
                return FakeFile();
            }
            node = findOrCreate(path, /*asDir=*/false, create);
            if (!node) {
                return FakeFile();
            }
        }
        if (mode != nullptr && mode[0] == 'w') {
            node->data.clear();
        }
        return FakeFile(node, path);
    }

private:
    std::shared_ptr<FakeFsNode> root_;
    size_t totalBytes_{262144};
    size_t usedBytes_{0};

    static std::vector<std::string> splitSegments(const std::string& path) {
        std::vector<std::string> segments;
        size_t start = !path.empty() && path[0] == '/' ? 1U : 0U;
        for (size_t i = start; i <= path.size(); ++i) {
            if (i == path.size() || path[i] == '/') {
                if (i > start) {
                    segments.push_back(path.substr(start, i - start));
                }
                start = i + 1U;
            }
        }
        return segments;
    }

    std::shared_ptr<FakeFsNode> find(const std::string& path) const {
        std::shared_ptr<FakeFsNode> cur = root_;
        for (const auto& segment : splitSegments(path)) {
            auto it = cur->children.find(segment);
            if (it == cur->children.end()) {
                return nullptr;
            }
            cur = it->second;
        }
        return cur;
    }

    std::pair<std::shared_ptr<FakeFsNode>, std::string> splitParent(const std::string& path) const {
        const std::vector<std::string> segments = splitSegments(path);
        if (segments.empty()) {
            return {nullptr, std::string()};
        }
        std::shared_ptr<FakeFsNode> cur = root_;
        for (size_t i = 0; i + 1U < segments.size(); ++i) {
            auto it = cur->children.find(segments[i]);
            if (it == cur->children.end()) {
                return {nullptr, std::string()};
            }
            cur = it->second;
        }
        return {cur, segments.back()};
    }

    // Creates every missing intermediate directory; the leaf is created as a directory if asDir,
    // otherwise as an empty file (only when createFile is true - matches FS::open's create flag).
    std::shared_ptr<FakeFsNode> findOrCreate(const std::string& path, const bool asDir, const bool createFile = true) {
        const std::vector<std::string> segments = splitSegments(path);
        if (segments.empty()) {
            return nullptr;
        }
        std::shared_ptr<FakeFsNode> cur = root_;
        for (size_t i = 0; i + 1U < segments.size(); ++i) {
            auto it = cur->children.find(segments[i]);
            if (it == cur->children.end()) {
                auto dir = std::make_shared<FakeFsNode>();
                dir->isDir = true;
                cur->children[segments[i]] = dir;
                cur = dir;
            } else {
                cur = it->second;
            }
        }
        const std::string& leaf = segments.back();
        auto it = cur->children.find(leaf);
        if (it != cur->children.end()) {
            return it->second;
        }
        if (!asDir && !createFile) {
            return nullptr;
        }
        auto node = std::make_shared<FakeFsNode>();
        node->isDir = asDir;
        cur->children[leaf] = node;
        return node;
    }
};

} // namespace test
} // namespace ewfm
