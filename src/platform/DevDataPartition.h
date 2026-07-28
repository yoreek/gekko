#pragma once

namespace ewfm {

// Dedicated device-data partition (my_partitions.csv), separate from the UI asset partition so
// `pio run -t uploadfs` cannot wipe runtime-generated data. App owns the single fs::LittleFSFS
// mount (see App::devDataFs_); consumers receive it by reference and never mount it themselves.
// Layout convention: one top-level directory per feature (dose journal "/dj", schedule presets
// "/sap"), one subdirectory per device id underneath - device ids are registry-unique, so names
// cannot collide.
constexpr const char* kDeviceDataPartitionLabel = "devdata";

} // namespace ewfm
