#pragma once

#include <ArduinoJson.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// Native-test-only JSON Schema smoke validator. It deliberately implements just the vocabulary
// used by Gekko's REST schemas. The firmware never includes this header.
namespace json_schema_smoke {

inline bool isInteger(const JsonVariantConst& value) {
    return value.is<int>() || value.is<unsigned int>() || value.is<long>() || value.is<unsigned long>() || value.is<int64_t>() ||
           value.is<uint64_t>();
}

inline bool valuesEqual(const JsonVariantConst& left, const JsonVariantConst& right) {
    if (left.is<bool>() || right.is<bool>()) {
        return left.is<bool>() && right.is<bool>() && left.as<bool>() == right.as<bool>();
    }
    if (left.is<const char*>() || right.is<const char*>()) {
        const char* leftText = left.as<const char*>();
        const char* rightText = right.as<const char*>();
        return leftText != nullptr && rightText != nullptr && std::strcmp(leftText, rightText) == 0;
    }
    if (isInteger(left) || isInteger(right)) {
        return isInteger(left) && isInteger(right) && left.as<int64_t>() == right.as<int64_t>();
    }
    return false;
}

inline bool loadSchema(const std::string& path, DynamicJsonDocument& doc, std::string& error) {
    std::ifstream input(path);
    if (!input.good()) {
        error = "cannot open schema " + path;
        return false;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    const DeserializationError parseError = deserializeJson(doc, contents.str());
    if (parseError) {
        error = "cannot parse schema " + path + ": " + parseError.c_str();
        return false;
    }
    return true;
}

inline bool validate(const JsonObjectConst& schema, const std::string& schemaPath, const JsonVariantConst& value, std::string& error);

inline bool validateReference(const char* reference, const std::string& schemaPath, const JsonVariantConst& value, std::string& error) {
    const std::filesystem::path referencedPath = std::filesystem::path(schemaPath).parent_path() / reference;
    DynamicJsonDocument referencedSchema(16384);
    if (!loadSchema(referencedPath.lexically_normal().string(), referencedSchema, error)) {
        return false;
    }
    return validate(referencedSchema.as<JsonObjectConst>(), referencedPath.lexically_normal().string(), value, error);
}

inline bool validateType(const char* type, const JsonVariantConst& value) {
    if (std::strcmp(type, "object") == 0) {
        return value.is<JsonObjectConst>();
    }
    if (std::strcmp(type, "array") == 0) {
        return value.is<JsonArrayConst>();
    }
    if (std::strcmp(type, "string") == 0) {
        return value.is<const char*>();
    }
    if (std::strcmp(type, "boolean") == 0) {
        return value.is<bool>();
    }
    if (std::strcmp(type, "integer") == 0) {
        return isInteger(value);
    }
    if (std::strcmp(type, "number") == 0) {
        return isInteger(value) || value.is<float>() || value.is<double>();
    }
    return false;
}

inline bool validate(const JsonObjectConst& schema, const std::string& schemaPath, const JsonVariantConst& value, std::string& error) {
    if (schema.isNull()) {
        error = "schema root must be an object";
        return false;
    }
    if (const char* reference = schema["$ref"]; reference != nullptr) {
        return validateReference(reference, schemaPath, value, error);
    }
    if (const char* type = schema["type"]; type != nullptr && !validateType(type, value)) {
        error = "value does not match type " + std::string(type);
        return false;
    }
    if (!schema["const"].isNull() && !valuesEqual(value, schema["const"])) {
        error = "value does not match const";
        return false;
    }
    if (const JsonArrayConst allowedValues = schema["enum"].as<JsonArrayConst>(); !allowedValues.isNull()) {
        bool matches = false;
        for (const JsonVariantConst allowed : allowedValues) {
            matches = matches || valuesEqual(value, allowed);
        }
        if (!matches) {
            error = "value is not in enum";
            return false;
        }
    }
    if (value.is<const char*>()) {
        const size_t length = std::strlen(value.as<const char*>());
        if (!schema["minLength"].isNull() && length < schema["minLength"].as<size_t>()) {
            error = "string is shorter than minLength";
            return false;
        }
        if (!schema["maxLength"].isNull() && length > schema["maxLength"].as<size_t>()) {
            error = "string exceeds maxLength";
            return false;
        }
    }
    if (isInteger(value) || value.is<float>() || value.is<double>()) {
        const double number = value.as<double>();
        if (!schema["minimum"].isNull() && number < schema["minimum"].as<double>()) {
            error = "number is below minimum";
            return false;
        }
        if (!schema["maximum"].isNull() && number > schema["maximum"].as<double>()) {
            error = "number exceeds maximum";
            return false;
        }
    }
    if (const JsonObjectConst forbidden = schema["not"].as<JsonObjectConst>(); !forbidden.isNull()) {
        std::string ignored;
        if (validate(forbidden, schemaPath, value, ignored)) {
            error = "value matches forbidden schema";
            return false;
        }
    }
    if (const JsonObjectConst object = value.as<JsonObjectConst>(); !object.isNull()) {
        if (const JsonArrayConst required = schema["required"].as<JsonArrayConst>(); !required.isNull()) {
            for (const JsonVariantConst requiredName : required) {
                const char* name = requiredName.as<const char*>();
                if (name == nullptr || object[name].isNull()) {
                    error = "required property is missing";
                    return false;
                }
            }
        }
        const JsonObjectConst properties = schema["properties"].as<JsonObjectConst>();
        for (JsonPairConst property : object) {
            const JsonObjectConst propertySchema = properties[property.key().c_str()].as<JsonObjectConst>();
            if (propertySchema.isNull()) {
                if (schema["additionalProperties"] == false) {
                    error = "additional property is not allowed";
                    return false;
                }
                continue;
            }
            if (!validate(propertySchema, schemaPath, property.value(), error)) {
                error = std::string(property.key().c_str()) + ": " + error;
                return false;
            }
        }
    }
    if (const JsonArrayConst array = value.as<JsonArrayConst>(); !array.isNull()) {
        const JsonObjectConst itemSchema = schema["items"].as<JsonObjectConst>();
        if (!itemSchema.isNull()) {
            for (const JsonVariantConst item : array) {
                if (!validate(itemSchema, schemaPath, item, error)) {
                    error = "array item: " + error;
                    return false;
                }
            }
        }
    }
    return true;
}

inline bool validateFile(const char* schemaPath, const JsonVariantConst& value, std::string& error) {
    DynamicJsonDocument schema(16384);
    if (!loadSchema(schemaPath, schema, error)) {
        return false;
    }
    return validate(schema.as<JsonObjectConst>(), schemaPath, value, error);
}

} // namespace json_schema_smoke
