#include "devices/DeviceTypes.h"

#include "devices/DummyDevice.h"

namespace ewfm {

bool DeviceTypeRegistry::registerDescriptor(const DeviceTypeDescriptor& descriptor) {
    if (descriptor.typeId == 0 || descriptor.name == nullptr) {
        return false;
    }

    if (find(descriptor.typeId) != nullptr) {
        return false;
    }

    descriptors_.push_back(descriptor);
    return true;
}

const DeviceTypeDescriptor* DeviceTypeRegistry::find(DeviceTypeId typeId) const {
    for (const auto& descriptor : descriptors_) {
        if (descriptor.typeId == typeId) {
            return &descriptor;
        }
    }
    return nullptr;
}

DeviceTypeRegistry DeviceTypeRegistry::withDefaults() {
    DeviceTypeRegistry registry;
    (void)registry.registerDescriptor(DummyDevice::descriptor());
    return registry;
}

} // namespace ewfm
