#pragma once

#include "devices/display/DisplayLayoutStore.h"

#include <ArduinoJson.h>
#include <cstddef>
#include <string>
#include <vector>

namespace ewfm {

// Base64 codec shared with the setup-bundle export/import ("layout_bitmap" NDJSON line in
// DisplayDeviceApiAdapter.cpp), which reads/writes a bitmap's raw bytes from/to the blob store and
// needs the same encoding used historically for the JSON "bitmapData" field.
bool copyStringToBytes(std::vector<uint8_t>& dest, const char* source);
std::string bytesToString(const std::vector<uint8_t>& bytes);

bool parseDisplayLayoutJson(const JsonObjectConst& input, DisplayLayoutRecordV1& layout);
bool parseDisplayLayoutWidgetJson(const JsonObjectConst& input, DisplayLayoutWidgetV1& widget);
// Serialize the layout into `output`. When `onlyPageIndex >= 0`, only that single page is emitted in
// the "pages" array (used by GET /api/devices/:id/layout?page=N); the default emits every page.
void writeDisplayLayoutJson(const DisplayLayoutRecordV1& layout, JsonObject output, int onlyPageIndex = -1);
// Serialize one page object (id/name/order/widgets) into `pageJson`.
void writeDisplayLayoutPageJson(const DisplayLayoutPageV1& page, JsonObject pageJson);
// Serialize one widget object into `widgetJson`. Lets the layout endpoint stream one widget per
// chunk, so the fixed serialization buffer only has to hold a single widget (its bitmap), not a
// whole page.
void writeDisplayLayoutWidgetJson(const DisplayLayoutWidgetV1& widget, JsonObject widgetJson);
// The layout's active page id, falling back to the first page then "main".
const char* displayLayoutActivePageId(const DisplayLayoutRecordV1& layout);
bool encodeDisplayLayoutBinary(const DisplayLayoutRecordV1& layout, std::vector<uint8_t>& blob);
bool decodeDisplayLayoutBinary(const uint8_t* data, size_t size, DisplayLayoutRecordV1& layout);

} // namespace ewfm
