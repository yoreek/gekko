#include "platform/PreferencesConfigStorage.h"

namespace ewfm {

bool PreferencesConfigStorage::begin(const char* ns, bool readOnly) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.begin(ns, readOnly);
#else
    (void)ns;
    (void)readOnly;
    return false;
#endif
}

void PreferencesConfigStorage::end() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    preferences_.end();
#endif
}

bool PreferencesConfigStorage::hasKey(const char* key) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.isKey(key);
#else
    (void)key;
    return false;
#endif
}

bool PreferencesConfigStorage::putString(const char* key, const std::string& value) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.putString(key, value.c_str()) == value.size();
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::getString(const char* key, std::string& value) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!preferences_.isKey(key)) {
        return false;
    }
    value = preferences_.getString(key, value.c_str()).c_str();
    return true;
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::putUInt(const char* key, uint32_t value) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.putUInt(key, value) == sizeof(uint32_t);
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::getUInt(const char* key, uint32_t& value) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!preferences_.isKey(key)) {
        return false;
    }
    value = preferences_.getUInt(key, value);
    return true;
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::putBool(const char* key, bool value) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.putBool(key, value) == sizeof(bool);
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::getBool(const char* key, bool& value) const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (!preferences_.isKey(key)) {
        return false;
    }
    value = preferences_.getBool(key, value);
    return true;
#else
    (void)key;
    (void)value;
    return false;
#endif
}

bool PreferencesConfigStorage::remove(const char* key) {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return preferences_.remove(key);
#else
    (void)key;
    return false;
#endif
}

} // namespace ewfm
