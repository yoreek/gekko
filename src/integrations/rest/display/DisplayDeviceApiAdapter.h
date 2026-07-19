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
    static bool collectLayoutMetricSourceDependencies(const DisplayLayoutRecordV1& layout,
                                                      std::array<DeviceDependencyLink, kMaxDeviceDependencies>& dependencies,
                                                      uint8_t& dependencyCount, const char*& error, const char* invalidLayoutError,
                                                      const char* dependencyCountError);
    static bool validateLayoutMetricPlaceholders(const DisplayLayoutRecordV1& layout, const DeviceRegistry& registry);
};

} // namespace ewfm
