#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

using String = std::string;

enum WebRequestMethod : uint8_t {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_PATCH,
    HTTP_DELETE,
    HTTP_OPTIONS,
    HTTP_ANY,
};
