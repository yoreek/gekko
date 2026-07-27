#include "integrations/rest/display/DisplayDeviceApiAdapter.h"

#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/DisplayLayoutCodec.h"
#include "devices/display/DisplayTextPlaceholderAst.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ewfm {
namespace {

constexpr size_t kWidgetJsonCapacity = 5120;
constexpr char kLayoutBeginKind[] = "layout_begin";
constexpr char kLayoutPageKind[] = "layout_page";
constexpr char kLayoutWidgetKind[] = "layout_widget";
constexpr char kLayoutEndKind[] = "layout_end";

class DisplayLayoutJsonProducer final : public IJsonChunkProducer {
public:
    DisplayLayoutJsonProducer(const DisplayLayoutRecordV1& layout, const int onlyPageIndex)
        : layout_(layout), onlyPageIndex_(onlyPageIndex) {}

    bool next(IJsonChunkSink& sink) override {
        if (phase_ == 0) {
            StaticJsonDocument<48> idDocument;
            idDocument.set(displayLayoutActivePageId(layout_));
            char idJson[48];
            const size_t idLength = serializeJson(idDocument, idJson, sizeof(idJson));
            char header[192];
            const char* prefix = "{\"success\":true,\"schemaVersion\":%u,\"activePageId\":%.*s,\"pages\":[";
            const int length = std::snprintf(header, sizeof(header), prefix, static_cast<unsigned>(layout_.schemaVersion),
                                             static_cast<int>(idLength), idJson);
            phase_ = 1;
            return length > 0 && sink.emit(header, static_cast<size_t>(length));
        }

        if (phase_ == 1) {
            while (pageCursor_ < layout_.pages.size()) {
                if (onlyPageIndex_ >= 0 && pageCursor_ != static_cast<size_t>(onlyPageIndex_)) {
                    ++pageCursor_;
                    continue;
                }
                const DisplayLayoutPageV1& page = layout_.pages[pageCursor_];
                if (!pageOpen_) {
                    StaticJsonDocument<128> metaDocument;
                    metaDocument["id"] = page.id;
                    metaDocument["name"] = page.name;
                    metaDocument["order"] = page.order;
                    char meta[128];
                    const size_t metaLength = serializeJson(metaDocument, meta, sizeof(meta));
                    char prefix[192];
                    const int length = std::snprintf(prefix, sizeof(prefix), "%s%.*s,\"widgets\":[", anyPageEmitted_ ? "," : "",
                                                     metaLength > 0U ? static_cast<int>(metaLength - 1U) : 0, meta);
                    pageOpen_ = true;
                    widgetCursor_ = 0;
                    anyWidgetEmitted_ = false;
                    anyPageEmitted_ = true;
                    return length > 0 && sink.emit(prefix, static_cast<size_t>(length));
                }

                if (widgetCursor_ < page.widgets.size()) {
                    widgetDocument_.clear();
                    writeDisplayLayoutWidgetJson(page.widgets[widgetCursor_++], widgetDocument_.to<JsonObject>());
                    if (widgetDocument_.overflowed() || !sink.emitJson(widgetDocument_, anyWidgetEmitted_)) {
                        continue;
                    }
                    anyWidgetEmitted_ = true;
                    return true;
                }

                pageOpen_ = false;
                ++pageCursor_;
                return sink.emit("]}", 2U);
            }
            phase_ = 2;
        }

        if (phase_ == 2) {
            phase_ = 3;
            return sink.emit("]}", 2U);
        }
        return false;
    }

private:
    DisplayLayoutRecordV1 layout_{};
    int onlyPageIndex_{-1};
    size_t pageCursor_{0};
    size_t widgetCursor_{0};
    bool pageOpen_{false};
    bool anyPageEmitted_{false};
    bool anyWidgetEmitted_{false};
    uint8_t phase_{0};
    StaticJsonDocument<kWidgetJsonCapacity> widgetDocument_{};
};

class DisplaySetupExportJsonProducer final : public IJsonChunkProducer {
public:
    explicit DisplaySetupExportJsonProducer(const DisplayLayoutRecordV1& layout) : layout_(layout) {}

    bool next(IJsonChunkSink& sink) override {
        document_.clear();
        JsonObject output = document_.to<JsonObject>();
        if (phase_ == 0U) {
            output["kind"] = kLayoutBeginKind;
            output["deviceId"] = layout_.deviceId;
            output["schemaVersion"] = layout_.schemaVersion;
            output["activePageId"] = displayLayoutActivePageId(layout_);
            output["pageCount"] = layout_.pages.size();
            phase_ = 1U;
            return emitLine(sink);
        }

        if (phase_ == 1U && pageIndex_ < layout_.pages.size()) {
            const DisplayLayoutPageV1& page = layout_.pages[pageIndex_];
            output["kind"] = kLayoutPageKind;
            output["deviceId"] = layout_.deviceId;
            output["pageIndex"] = pageIndex_;
            output["id"] = page.id;
            output["name"] = page.name;
            output["order"] = page.order;
            output["widgetCount"] = page.widgets.size();
            widgetIndex_ = 0U;
            phase_ = 2U;
            return emitLine(sink);
        }

        if (phase_ == 2U) {
            const DisplayLayoutPageV1& page = layout_.pages[pageIndex_];
            if (widgetIndex_ < page.widgets.size()) {
                output["kind"] = kLayoutWidgetKind;
                output["deviceId"] = layout_.deviceId;
                output["pageIndex"] = pageIndex_;
                output["widgetIndex"] = widgetIndex_;
                writeDisplayLayoutWidgetJson(page.widgets[widgetIndex_], output);
                ++widgetIndex_;
                return emitLine(sink);
            }
            ++pageIndex_;
            phase_ = 1U;
            return next(sink);
        }

        if (phase_ == 1U) {
            output["kind"] = kLayoutEndKind;
            output["deviceId"] = layout_.deviceId;
            phase_ = 3U;
            return emitLine(sink);
        }
        return false;
    }

private:
    bool emitLine(IJsonChunkSink& sink) {
        if (document_.overflowed()) {
            return false;
        }
        line_.clear();
        serializeJson(document_, line_);
        line_ += '\n';
        return sink.emit(line_.data(), line_.size());
    }

    DisplayLayoutRecordV1 layout_{};
    size_t pageIndex_{0U};
    size_t widgetIndex_{0U};
    uint8_t phase_{0U};
    StaticJsonDocument<kWidgetJsonCapacity> document_{};
    std::string line_{};
};

const DisplayDeviceBase& displayRuntime(const IDeviceRuntime& runtime) {
    return static_cast<const DisplayDeviceBase&>(runtime);
}

} // namespace

class DisplaySetupImportSession final : public IDeviceSetupImportSession {
public:
    explicit DisplaySetupImportSession(const DeviceId deviceId) : deviceId_(deviceId) {
        layout_.deviceId = deviceId;
        layout_.recordVersion = kDisplayLayoutRecordVersion;
    }

    bool consumeRecord(const JsonObjectConst& input, const char*& error) override {
        const char* kind = input["kind"] | "";
        if (!beginSeen_) {
            return consumeBegin(input, kind, error);
        }
        if (complete_) {
            error = "display layout contains records after layout_end";
            return false;
        }
        if (currentWidgetCount_ < expectedWidgetCount_) {
            return consumeWidget(input, kind, error);
        }
        if (layout_.pages.size() < expectedPageCount_) {
            return consumePage(input, kind, error);
        }
        if (std::strcmp(kind, kLayoutEndKind) != 0) {
            error = "display layout_end record is required";
            return false;
        }
        if (!hasExpectedDeviceId(input)) {
            error = "display layout deviceId does not match its device";
            return false;
        }
        complete_ = true;
        error = nullptr;
        return true;
    }

    bool complete() const override {
        return complete_;
    }

    bool finalize(DeviceCreateRequest& request, DeviceCreatePersistenceRequest& persistedRequest, const char*& error) override {
        persistedRequest = {};
        if (!complete_) {
            error = "display layout is incomplete";
            return false;
        }
        if (layout_.pages.empty()) {
            error = nullptr;
            return true;
        }

        bool activePageFound = false;
        for (size_t index = 0; index < layout_.pages.size(); ++index) {
            if (std::strcmp(layout_.pages[index].id, activePageId_) == 0) {
                layout_.activePageIndex = static_cast<uint8_t>(index);
                activePageFound = true;
                break;
            }
        }
        if (!activePageFound) {
            error = "display layout activePageId is invalid";
            return false;
        }

        std::vector<uint8_t> encoded;
        if (!encodeDisplayLayoutBinary(layout_, encoded) || !persistedRequest.persistedStateBlob.assign(encoded)) {
            error = "display layout exceeds supported size";
            return false;
        }
        if (!DisplayDeviceApiAdapter::collectLayoutMetricSourceDependencies(layout_, request.deps, request.depCount, error,
                                                                            "display layout is invalid",
                                                                            "display layout dependencies exceed supported count")) {
            return false;
        }
        persistedRequest.persistedStateProvided = true;
        error = nullptr;
        return true;
    }

private:
    bool consumeBegin(const JsonObjectConst& input, const char* kind, const char*& error) {
        if (std::strcmp(kind, kLayoutBeginKind) != 0) {
            error = "display layout_begin record is required after device";
            return false;
        }
        if (!hasExpectedDeviceId(input)) {
            error = "display layout deviceId does not match its device";
            return false;
        }
        const JsonVariantConst pageCountInput = input["pageCount"];
        const size_t pageCount = pageCountInput | 0U;
        if (pageCountInput.isNull() || pageCount > kDisplayLayoutMaxPages) {
            error = "display layout pageCount is invalid";
            return false;
        }
        const uint8_t schemaVersion = input["schemaVersion"] | 0U;
        if (schemaVersion != 1U && schemaVersion != kDisplayLayoutSchemaVersion) {
            error = "display layout schemaVersion is unsupported";
            return false;
        }
        if (!copyText(activePageId_, sizeof(activePageId_), input["activePageId"] | "")) {
            error = "display layout activePageId is invalid";
            return false;
        }
        layout_.schemaVersion = kDisplayLayoutSchemaVersion;
        layout_.pages.reserve(pageCount);
        expectedPageCount_ = pageCount;
        beginSeen_ = true;
        error = nullptr;
        return true;
    }

    bool consumePage(const JsonObjectConst& input, const char* kind, const char*& error) {
        if (std::strcmp(kind, kLayoutPageKind) != 0) {
            error = "display layout_page record is required";
            return false;
        }
        if (!hasExpectedDeviceId(input) || (input["pageIndex"] | static_cast<size_t>(-1)) != layout_.pages.size()) {
            error = "display layout page order is invalid";
            return false;
        }
        const size_t widgetCount = input["widgetCount"] | static_cast<size_t>(kDisplayLayoutMaxWidgetsPerPage + 1U);
        if (widgetCount > kDisplayLayoutMaxWidgetsPerPage) {
            error = "display layout widgetCount is invalid";
            return false;
        }

        DisplayLayoutPageV1 page{};
        if (!copyText(page.id, sizeof(page.id), input["id"] | "") || page.id[0] == '\0' ||
            !copyText(page.name, sizeof(page.name), input["name"] | "") || page.name[0] == '\0') {
            error = "display layout page metadata is invalid";
            return false;
        }
        page.order = input["order"] | static_cast<uint8_t>(layout_.pages.size());
        page.widgets.reserve(widgetCount);
        layout_.pages.push_back(std::move(page));
        expectedWidgetCount_ = widgetCount;
        currentWidgetCount_ = 0U;
        error = nullptr;
        return true;
    }

    bool consumeWidget(const JsonObjectConst& input, const char* kind, const char*& error) {
        if (std::strcmp(kind, kLayoutWidgetKind) != 0) {
            error = "display layout_widget record is required";
            return false;
        }
        const size_t pageIndex = layout_.pages.size() - 1U;
        if (!hasExpectedDeviceId(input) || (input["pageIndex"] | static_cast<size_t>(-1)) != pageIndex ||
            (input["widgetIndex"] | static_cast<size_t>(-1)) != currentWidgetCount_) {
            error = "display layout widget order is invalid";
            return false;
        }
        DisplayLayoutWidgetV1 widget{};
        if (!parseDisplayLayoutWidgetJson(input, widget)) {
            error = "display layout widget is invalid";
            return false;
        }
        layout_.pages.back().widgets.push_back(std::move(widget));
        ++currentWidgetCount_;
        error = nullptr;
        return true;
    }

    bool hasExpectedDeviceId(const JsonObjectConst& input) const {
        return static_cast<DeviceId>(input["deviceId"] | 0U) == deviceId_;
    }

    static bool copyText(char* destination, const size_t capacity, const char* source) {
        if (destination == nullptr || capacity == 0U || source == nullptr || std::strlen(source) >= capacity) {
            return false;
        }
        std::memcpy(destination, source, std::strlen(source) + 1U);
        return true;
    }

    DeviceId deviceId_{0U};
    DisplayLayoutRecordV1 layout_{};
    char activePageId_[kDisplayLayoutPageIdCapacity]{};
    size_t expectedPageCount_{0U};
    size_t expectedWidgetCount_{0U};
    size_t currentWidgetCount_{0U};
    bool beginSeen_{false};
    bool complete_{false};
};

std::unique_ptr<IJsonChunkProducer> DisplayDeviceApiAdapter::createLayoutJsonProducer(const IDeviceRuntime& runtime,
                                                                                      const int onlyPageIndex) const {
    return std::make_unique<DisplayLayoutJsonProducer>(displayRuntime(runtime).layout(), onlyPageIndex);
}

std::unique_ptr<IJsonChunkProducer> DisplayDeviceApiAdapter::createSetupExportJsonProducer(const IDeviceRuntime& runtime) const {
    return std::make_unique<DisplaySetupExportJsonProducer>(displayRuntime(runtime).layout());
}

std::unique_ptr<IDeviceSetupImportSession> DisplayDeviceApiAdapter::createSetupImportSession(const DeviceId deviceId) const {
    return std::make_unique<DisplaySetupImportSession>(deviceId);
}

bool DisplayDeviceApiAdapter::encodeLayoutRequest(const JsonObjectConst& input, const DeviceId deviceId,
                                                  BoundedBlob<kMaxDisplayLayoutBytes>& blob, const char*& error,
                                                  const char* invalidLayoutError, const char* layoutSizeError) {
    blob.clear();
    const JsonObjectConst layoutInput = input["config"]["layout"].as<JsonObjectConst>();
    if (layoutInput.isNull()) {
        error = nullptr;
        return true;
    }

    DisplayLayoutRecordV1 layout{};
    if (!parseDisplayLayoutJson(layoutInput, layout)) {
        error = invalidLayoutError;
        return false;
    }
    layout.deviceId = deviceId;

    std::vector<uint8_t> encoded;
    if (!encodeDisplayLayoutBinary(layout, encoded)) {
        error = invalidLayoutError;
        return false;
    }
    if (!blob.assign(encoded)) {
        error = layoutSizeError;
        return false;
    }
    error = nullptr;
    return true;
}

bool DisplayDeviceApiAdapter::collectLayoutMetricSourceDependencies(const DisplayLayoutRecordV1& layout,
                                                                    std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                                                    uint8_t& dependencyCount, const char*& error,
                                                                    const char* invalidLayoutError, const char* dependencyCountError) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (!isDisplayTextWidgetType(static_cast<DisplayLayoutWidgetType>(widget.type))) {
                continue;
            }
            if (!collectTextPlaceholderDeviceIds(widget.text, dependencies, dependencyCount, error, invalidLayoutError,
                                                 dependencyCountError)) {
                return false;
            }
        }
    }
    return true;
}

bool DisplayDeviceApiAdapter::validateLayoutMetricPlaceholders(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry) {
    for (const DisplayLayoutPageV1& page : layout.pages) {
        for (const DisplayLayoutWidgetV1& widget : page.widgets) {
            if (!isDisplayTextWidgetType(static_cast<DisplayLayoutWidgetType>(widget.type))) {
                continue;
            }
            if (!validateDisplayTextWidget(widget, registry).ok()) {
                return false;
            }
        }
    }
    return true;
}

} // namespace ewfm
