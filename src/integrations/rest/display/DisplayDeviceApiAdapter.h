#pragma once

#include "devices/display/DisplayLayoutStore.h"
#include "integrations/common/DeviceApiAdapter.h"

namespace ewfm {

class DisplaySetupImportSession;

class DisplayDeviceApiAdapter : public IDeviceApiAdapter {
public:
    std::unique_ptr<IJsonChunkProducer> createLayoutJsonProducer(const IDeviceRuntime& runtime, int onlyPageIndex = -1) const final;
    std::unique_ptr<IJsonChunkProducer> createSetupExportJsonProducer(const IDeviceRuntime& runtime) const final;
    std::unique_ptr<IDeviceSetupImportSession> createSetupImportSession(DeviceId deviceId) const final;

protected:
    friend class DisplaySetupImportSession;

    static bool encodeLayoutRequest(const JsonObjectConst& input, DeviceId deviceId, BoundedBlob<kMaxDisplayLayoutBytes>& blob,
                                    const char*& error, const char* invalidLayoutError, const char* layoutSizeError);
    static bool parseAndEncodeLayoutRequest(const JsonObjectConst& input, DeviceId deviceId, BoundedBlob<kMaxDisplayLayoutBytes>& blob,
                                            std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                            uint8_t& dependencyCount, const char*& error, const char* invalidLayoutError,
                                            const char* layoutSizeError, const char* dependencyCountError);
    static bool collectLayoutMetricSourceDependencies(const DisplayLayoutRecordV1& layout,
                                                      std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                                      uint8_t& dependencyCount, const char*& error, const char* invalidLayoutError,
                                                      const char* dependencyCountError);
    static bool validateLayoutMetricPlaceholders(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry);
    // Every Bitmap widget's imageKey (if non-empty) must reference a blob that actually exists in
    // the blob store and is within the display-layout bitmap size ceiling - the create/update
    // request only carries the key, not the bytes, so this is the one place that can catch a
    // dangling/oversized reference before it is persisted.
    static bool validateLayoutImageKeys(const DisplayLayoutRecordV1& layout, const char*& error);
};

} // namespace ewfm
