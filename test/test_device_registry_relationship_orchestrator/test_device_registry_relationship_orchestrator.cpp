#include "devices/registry/DeviceRegistryRelationshipOrchestrator.h"

#include <algorithm>
#include <unity.h>

using namespace ewfm;

namespace {

struct LinkRuntime final : public IDeviceRuntime {
    void begin(uint32_t) override {}
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    void setParentRuntime(IDeviceRuntime* parentRuntime) override {
        parentRuntime_ = parentRuntime;
    }
    IDeviceRuntime* parentRuntime() const override {
        return parentRuntime_;
    }
    void attachChildRuntime(IDeviceRuntime* childRuntime) override {
        if (childRuntime == nullptr || std::find(childRuntimes_.begin(), childRuntimes_.end(), childRuntime) != childRuntimes_.end()) {
            return;
        }
        childRuntimes_.push_back(childRuntime);
    }
    void detachChildRuntime(IDeviceRuntime* childRuntime) override {
        const auto it = std::remove(childRuntimes_.begin(), childRuntimes_.end(), childRuntime);
        childRuntimes_.erase(it, childRuntimes_.end());
    }
    const std::vector<IDeviceRuntime*>& childRuntimes() const override {
        return childRuntimes_;
    }
    void requestReconfigure() override {
        reconfigureCount += 1;
    }
    void requestDisable() override {
        disableCount += 1;
    }
    void requestDelete() override {}
    DeviceStatus status() const override {
        return status_;
    }
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }

    DeviceStatus status_{DeviceStatus::Unknown};
    IDeviceRuntime* parentRuntime_{nullptr};
    std::vector<IDeviceRuntime*> childRuntimes_{};
    uint32_t reconfigureCount{0};
    uint32_t disableCount{0};
};

DeviceRecord makeRecord(DeviceId id, bool enabled, bool hasParent, DeviceId parentId, DeviceStatus status = DeviceStatus::Ready) {
    DeviceRecord record{};
    record.header.deviceId = id;
    record.header.typeId = 1;
    record.enabled = enabled;
    record.hasParent = hasParent;
    record.parentDeviceId = parentId;
    record.status = status;
    return record;
}

} // namespace

void test_relationship_orchestrator_syncs_links_and_dependency_states() {
    DeviceRegistrySnapshot snapshot{};
    snapshot.records.push_back(makeRecord(1, true, false, 0, DeviceStatus::Ready));
    snapshot.records.push_back(makeRecord(2, true, true, 1, DeviceStatus::Ready));
    snapshot.indexEntries.push_back({1, 1});
    snapshot.indexEntries.push_back({2, 1});

    DeviceRuntimeMap runtimes{};
    auto parent = std::unique_ptr<LinkRuntime>(new LinkRuntime{});
    parent->status_ = DeviceStatus::Ready;
    auto child = std::unique_ptr<LinkRuntime>(new LinkRuntime{});
    child->status_ = DeviceStatus::DependencyBlocked;

    LinkRuntime* parentPtr = parent.get();
    LinkRuntime* childPtr = child.get();

    DeviceRuntimeSlot parentSlot{};
    parentSlot.runtime = std::move(parent);
    DeviceRuntimeSlot childSlot{};
    childSlot.runtime = std::move(child);
    runtimes.emplace(1, std::move(parentSlot));
    runtimes.emplace(2, std::move(childSlot));

    DeviceRegistryRelationshipOrchestrator::syncRuntimeParentLink(2, snapshot, runtimes);
    TEST_ASSERT_EQUAL_PTR(parentPtr, childPtr->parentRuntime());
    TEST_ASSERT_EQUAL_UINT32(1, parentPtr->childRuntimes().size());
    TEST_ASSERT_EQUAL_PTR(childPtr, parentPtr->childRuntimes()[0]);

    DeviceRegistryRelationshipOrchestrator::refreshDependentRuntimeStates(snapshot, runtimes);
    TEST_ASSERT_EQUAL_UINT32(1, childPtr->reconfigureCount);
    TEST_ASSERT_EQUAL_UINT32(0, childPtr->disableCount);

    parentPtr->status_ = DeviceStatus::Faulted;
    const DeviceStatus childEffective =
        DeviceRegistryRelationshipOrchestrator::effectiveStatusForRecord(snapshot.records[1], snapshot, runtimes);
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(DeviceStatus::DependencyBlocked), static_cast<uint32_t>(childEffective));

    snapshot.records[1].enabled = false;
    DeviceRegistryRelationshipOrchestrator::refreshDependentRuntimeStates(snapshot, runtimes);
    TEST_ASSERT_EQUAL_UINT32(1, childPtr->disableCount);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_relationship_orchestrator_syncs_links_and_dependency_states);
    return UNITY_END();
}
