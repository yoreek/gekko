#pragma once

#include "platform/BlobKeyValidation.h"
#include "platform/DevDataPartition.h"
#include "platform/RandomBlobKey.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <LittleFS.h>
#endif

namespace ewfm {

// LittleFS needs spare blocks for wear-leveled writes; refuse a write rather than run the devdata
// partition (shared with the dose journal and schedule presets) down to zero. Mirrors
// kDoseJournalMinFreeBytes / kSchedulePresetMinFreeBytes.
constexpr size_t kBlobStoreMinFreeBytes = 8192;

// Reserved top-level directory this store owns exclusively on the shared devdata partition -
// distinct from the dose journal's "/dj" and schedule presets' "/sap". Every key is confined
// under this root, so a generic REST client passing an arbitrary key can never reach outside the
// store's own namespace and touch another feature's data.
constexpr const char* kBlobStoreRootDir = "/blobs";

// Generic key -> byte-blob object store on the shared devdata partition (my_partitions.csv).
// Keys map 1:1 onto filesystem paths under kBlobStoreRootDir (e.g. "foo/42/bar" ->
// "/blobs/foo/42/bar"); the store has no concept of any particular feature/consumer.
//
// Templated on the filesystem (Fs) and file (FileT) types so the exact same algorithm - not a
// reimplementation of it - can be exercised in native unit tests against an in-memory fake
// (test/fakes/littlefs/FakeLittleFs.h), not just compiled for real hardware. Fs must provide
// open/exists/mkdir/rmdir/remove/rename/totalBytes/usedBytes matching fs::LittleFSFS's signatures;
// FileT must provide operator bool/write/read/close/isDirectory/openNextFile/path/size matching
// fs::File's, and be default-constructible (falsy) and movable.
template <typename Fs, typename FileT> class LittleFsBlobStoreCore {
public:
    explicit LittleFsBlobStoreCore(Fs& fs) : fs_(fs) {}

    // Assumes the caller has already mounted the shared devdata partition; creates the store's
    // reserved root directory (kBlobStoreRootDir) if missing.
    bool begin() {
        if (!fs_.exists(kBlobStoreRootDir)) {
            return fs_.mkdir(kBlobStoreRootDir);
        }
        return true;
    }

    // A single in-progress write, opened by beginPut(). Move-only. Writes go to a temporary path
    // and are only renamed onto the final key's path by commit() - an aborted or never-committed
    // handle (including one destroyed without a call to commit()) removes its own temp file and
    // leaves any existing blob at that key untouched.
    class WriteHandle {
    public:
        WriteHandle() = default;
        WriteHandle(WriteHandle&& other) noexcept {
            *this = std::move(other);
        }
        WriteHandle& operator=(WriteHandle&& other) noexcept {
            if (this != &other) {
                abort();
                fs_ = other.fs_;
                finalPath_ = std::move(other.finalPath_);
                tmpPath_ = std::move(other.tmpPath_);
                file_ = std::move(other.file_);
                active_ = other.active_;
                other.reset();
            }
            return *this;
        }
        WriteHandle(const WriteHandle&) = delete;
        WriteHandle& operator=(const WriteHandle&) = delete;
        ~WriteHandle() {
            abort();
        }

        bool valid() const {
            return active_;
        }

        bool write(const uint8_t* data, const size_t len) {
            if (!active_ || (data == nullptr && len > 0U)) {
                return false;
            }
            return file_.write(data, len) == len;
        }

        // Renames the temp file onto the final path (atomic replace of any existing blob at that
        // key). Invalidates the handle either way (success or failure).
        bool commit() {
            if (!active_) {
                return false;
            }
            file_.close();
            const bool ok = fs_->rename(tmpPath_.c_str(), finalPath_.c_str());
            reset();
            return ok;
        }

        // Explicit cancel: removes the temp file, invalidates the handle. Also what the
        // destructor does if commit() was never called.
        void abort() {
            if (!active_) {
                return;
            }
            file_.close();
            (void)fs_->remove(tmpPath_.c_str());
            reset();
        }

    private:
        friend class LittleFsBlobStoreCore;

        WriteHandle(Fs& fs, std::string finalPath, std::string tmpPath, FileT file)
            : fs_(&fs), finalPath_(std::move(finalPath)), tmpPath_(std::move(tmpPath)), file_(std::move(file)), active_(true) {}

        void reset() {
            fs_ = nullptr;
            finalPath_.clear();
            tmpPath_.clear();
            file_ = FileT();
            active_ = false;
        }

        Fs* fs_{nullptr};
        std::string finalPath_{};
        std::string tmpPath_{};
        FileT file_{};
        bool active_{false};
    };

    // Returns an invalid handle (valid() == false) if the key fails validation or the free-space
    // guard trips - callers must check valid() before writing.
    WriteHandle beginPut(const std::string& key) {
        if (!isValidBlobKey(key)) {
            return WriteHandle{};
        }
        const size_t total = fs_.totalBytes();
        const size_t used = fs_.usedBytes();
        if (total <= used || total - used < kBlobStoreMinFreeBytes) {
            return WriteHandle{};
        }

        const std::string finalPath = toPath(key);
        const std::string tmpPath = tmpPathFor(finalPath);
        if (fs_.exists(tmpPath.c_str())) {
            (void)fs_.remove(tmpPath.c_str());
        }
        // create=true: FS::open() walks and mkdir's every missing intermediate directory itself
        // (vfs_api.cpp), so arbitrarily nested keys need no manual mkdir-per-segment loop.
        FileT file = fs_.open(tmpPath.c_str(), "w", true);
        if (!file) {
            return WriteHandle{};
        }
        return WriteHandle(fs_, finalPath, tmpPath, std::move(file));
    }

    // Buffered read for small values. Returns false if the key is invalid, missing, or larger
    // than outCapacity.
    bool get(const std::string& key, uint8_t* out, const size_t outCapacity, size_t& outLen) const {
        outLen = 0U;
        if (!isValidBlobKey(key) || out == nullptr) {
            return false;
        }
        FileT file = fs_.open(toPath(key).c_str(), "r");
        if (!file || file.isDirectory()) {
            return false;
        }
        const size_t fileSize = file.size();
        if (fileSize > outCapacity) {
            file.close();
            return false;
        }
        const size_t read = file.read(out, fileSize);
        file.close();
        if (read != fileSize) {
            return false;
        }
        outLen = read;
        return true;
    }

    // Direct file handle for streaming HTTP responses (e.g. request->beginResponse(file, ...)) -
    // never buffers the blob into RAM. Returned FileT is falsy if the key is invalid or missing.
    FileT openForRead(const std::string& key) const {
        if (!isValidBlobKey(key)) {
            return FileT();
        }
        FileT file = fs_.open(toPath(key).c_str(), "r");
        if (!file || file.isDirectory()) {
            return FileT();
        }
        return file;
    }

    bool exists(const std::string& key) const {
        return isValidBlobKey(key) && fs_.exists(toPath(key).c_str());
    }

    size_t size(const std::string& key) const {
        if (!isValidBlobKey(key)) {
            return 0U;
        }
        FileT file = fs_.open(toPath(key).c_str(), "r");
        if (!file || file.isDirectory()) {
            return 0U;
        }
        const size_t fileSize = file.size();
        file.close();
        return fileSize;
    }

    bool remove(const std::string& key) {
        if (!isValidBlobKey(key)) {
            return false;
        }
        const std::string path = toPath(key);
        if (!fs_.exists(path.c_str())) {
            return true;
        }
        return fs_.remove(path.c_str());
    }

    // Generates a fresh, collision-checked key as `prefix + "/" + <random suffix>` and opens a
    // WriteHandle for it - the untrusted-upload counterpart to beginPut(key), for callers (e.g. a
    // REST client) that must never choose the unique part of a key themselves. `outKey` is set to
    // the generated key iff generation itself succeeded, regardless of whether the returned handle
    // is valid - this lets a caller tell "could not find a free key" (outKey empty) apart from
    // "found one but couldn't open it for writing" (outKey set, handle invalid).
    WriteHandle beginPutGenerated(const std::string& prefix, std::string& outKey) {
        std::string key;
        if (!generateUniqueBlobKey(
                prefix, &randomAlnumChar, [this](const std::string& candidate) { return fs_.exists(toPath(candidate).c_str()); }, key)) {
            outKey.clear();
            return WriteHandle{};
        }
        outKey = key;
        return beginPut(key);
    }

    // Recursively removes every file/subdirectory under `prefix`, then `prefix` itself. No-op
    // success if `prefix` does not exist. If `prefix` names a plain file (not a directory), the
    // file itself is removed - a "delete everything under this prefix" contract shouldn't require
    // callers to know in advance whether their prefix happens to be leaf- or subtree-shaped.
    bool removeByPrefix(const std::string& prefix) {
        if (!isValidBlobKey(prefix)) {
            return false;
        }
        return removeTree(toPath(prefix));
    }

    // Deletes everything the store owns. Deliberately its own method, not removeByPrefix("") -
    // isValidBlobKey rejects "", so an accidental empty-prefix call cannot wipe the devdata
    // partition out from under the dose journal / schedule preset stores it shares the mount with.
    bool wipeAll() {
        // removeTree() itself would rmdir() kBlobStoreRootDir away entirely, so recreate it after.
        return removeTree(kBlobStoreRootDir) && fs_.mkdir(kBlobStoreRootDir);
    }

private:
    static std::string toPath(const std::string& key) {
        return std::string(kBlobStoreRootDir) + "/" + key;
    }

    static std::string tmpPathFor(const std::string& path) {
        return path + ".tmp";
    }

    bool removeTree(const std::string& path) {
        if (!fs_.exists(path.c_str())) {
            return true; // prefix doesn't exist: no-op success
        }
        FileT entry = fs_.open(path.c_str());
        if (!entry) {
            return false;
        }
        if (!entry.isDirectory()) {
            entry.close();
            return fs_.remove(path.c_str());
        }

        // Snapshot every child (path + isDir) before removing anything - deleting entries while an
        // openNextFile() iteration is still in progress over the same directory is unsafe: POSIX
        // leaves readdir() behavior unspecified if the directory is mutated mid-iteration, and this
        // was caught concretely by a crash in the in-memory test fake, whose cursor-based iteration
        // over a std::map corrupted once entries were erased out from under it.
        std::vector<std::pair<std::string, bool>> children;
        for (FileT child = entry.openNextFile(); child; child = entry.openNextFile()) {
            // File::path() returns the full path (joined at openNextFile() time); File::name()
            // would only give the basename, which remove()/rmdir()/open() cannot resolve alone.
            children.emplace_back(child.path(), child.isDirectory());
            child.close();
        }
        entry.close();

        bool ok = true;
        for (const auto& entryPair : children) {
            const std::string& childPath = entryPair.first;
            const bool childIsDir = entryPair.second;
            ok = (childIsDir ? removeTree(childPath) : fs_.remove(childPath.c_str())) && ok;
        }
        if (!ok) {
            return false; // leave the now-partially-emptied directory in place; caller can retry
        }
        return fs_.rmdir(path.c_str()); // requires an empty directory - guaranteed by the loop above
    }

    Fs& fs_;
};

#if defined(ARDUINO) && !defined(UNIT_TEST)
using LittleFsBlobStore = LittleFsBlobStoreCore<fs::LittleFSFS, File>;
#else
// No fake filesystem is wired up for the production App under UNIT_TEST (real behavior is
// exercised via LittleFsBlobStoreCore instantiated against test/fakes/littlefs/FakeLittleFs.h
// instead, see test/test_core/test_littlefs_blob_store.cpp) - this stub exists only so App.h
// compiles under the native test build (test_build_src=yes compiles all of src/, including App.h).
class LittleFsBlobStore {
public:
    bool begin() {
        return true;
    }

    class WriteHandle {
    public:
        bool valid() const {
            return false;
        }
        bool write(const uint8_t*, size_t) {
            return false;
        }
        bool commit() {
            return false;
        }
        void abort() {}
    };

    WriteHandle beginPut(const std::string&) {
        return WriteHandle{};
    }
    WriteHandle beginPutGenerated(const std::string&, std::string& outKey) {
        outKey.clear();
        return WriteHandle{};
    }
    bool get(const std::string&, uint8_t*, size_t, size_t& outLen) const {
        outLen = 0U;
        return false;
    }
    bool exists(const std::string&) const {
        return false;
    }
    size_t size(const std::string&) const {
        return 0U;
    }
    bool remove(const std::string&) {
        return false;
    }
    bool removeByPrefix(const std::string&) {
        return false;
    }
    bool wipeAll() {
        return false;
    }
};
#endif

LittleFsBlobStore* defaultBlobStore();
void setDefaultBlobStore(LittleFsBlobStore* store);

} // namespace ewfm
