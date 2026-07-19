#pragma once

namespace ewfm::ha {

namespace key {
inline constexpr char kBaseTopic[] = "~";
inline constexpr char kUniqueId[] = "uniq_id";
inline constexpr char kObjectId[] = "object_id";
inline constexpr char kName[] = "name";
inline constexpr char kStateTopic[] = "stat_t";
inline constexpr char kCommandTopic[] = "cmd_t";
inline constexpr char kAvailabilityTopic[] = "avty_t";
inline constexpr char kHasEntityName[] = "has_entity_name";
inline constexpr char kDevice[] = "dev";
inline constexpr char kIdentifiers[] = "ids";
inline constexpr char kManufacturer[] = "mf";
inline constexpr char kOrigin[] = "o";
inline constexpr char kIcon[] = "ic";
inline constexpr char kDeviceClass[] = "dev_cla";
inline constexpr char kUnitOfMeasurement[] = "unit_of_meas";
inline constexpr char kStateClass[] = "stat_cla";
inline constexpr char kEntityCategory[] = "ent_cat";
inline constexpr char kPayloadOn[] = "pl_on";
inline constexpr char kPayloadOff[] = "pl_off";
inline constexpr char kStateOn[] = "stat_on";
inline constexpr char kStateOff[] = "stat_off";
inline constexpr char kOptions[] = "ops";
inline constexpr char kModes[] = "modes";
inline constexpr char kModeStateTopic[] = "mode_stat_t";
inline constexpr char kModeCommandTopic[] = "mode_cmd_t";
inline constexpr char kTemperatureStateTopic[] = "temp_stat_t";
inline constexpr char kTemperatureCommandTopic[] = "temp_cmd_t";
inline constexpr char kCurrentTemperatureTopic[] = "curr_temp_t";
inline constexpr char kActionTopic[] = "act_t";
inline constexpr char kTemperatureUnit[] = "temp_unit";
inline constexpr char kMinTemperature[] = "min_temp";
inline constexpr char kMaxTemperature[] = "max_temp";
inline constexpr char kTemperatureStep[] = "temp_step";
inline constexpr char kPrecision[] = "precision";
} // namespace key

namespace component {
inline constexpr char kSensor[] = "sensor";
inline constexpr char kBinarySensor[] = "binary_sensor";
inline constexpr char kSwitch[] = "switch";
inline constexpr char kClimate[] = "climate";
inline constexpr char kButton[] = "button";
inline constexpr char kLight[] = "light";
} // namespace component

namespace topic {
inline constexpr char kState[] = "state";
inline constexpr char kSet[] = "set";
} // namespace topic

namespace payload {
inline constexpr char kOn[] = "ON";
inline constexpr char kOff[] = "OFF";
} // namespace payload

} // namespace ewfm::ha
