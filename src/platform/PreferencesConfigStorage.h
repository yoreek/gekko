#pragma once

#include "config/ConfigStore.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Preferences.h>
#endif

namespace ewfm {

class PreferencesConfigStorage final : public IConfigStorage {
public:
    bool begin(const char* ns, bool readOnly) override;
    void end() override;
    bool hasKey(const char* key) const override;
    bool putString(const char* key, const std::string& value) override;
    bool getString(const char* key, std::string& value) const override;
    bool putBlob(const char* key, const std::vector<uint8_t>& value) override;
    bool getBlob(const char* key, std::vector<uint8_t>& value) const override;
    bool putUInt(const char* key, uint32_t value) override;
    bool getUInt(const char* key, uint32_t& value) const override;
    bool putBool(const char* key, bool value) override;
    bool getBool(const char* key, bool& value) const override;
    bool remove(const char* key) override;

private:
#if defined(ARDUINO) && !defined(UNIT_TEST)
    mutable Preferences preferences_;
#endif
};

} // namespace ewfm
