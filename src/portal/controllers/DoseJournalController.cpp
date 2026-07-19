#include "portal/controllers/DoseJournalController.h"

#include "time/DateTime.h"

#include <cstdio>
#include <memory>
#include <vector>

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#endif

namespace ewfm {

namespace {
constexpr uint32_t kDefaultPeriodDays = 7U;
constexpr uint32_t kMaxPeriodDays = 365U;
constexpr size_t kMaxHistoryEntries = 1000U;
constexpr uint16_t kDoseJournalMinValidYear = 2020U;
} // namespace

DoseJournalController::DoseJournalController(AsyncWebServerRequest* request, const Action action, IDoseJournal* journal)
    : BaseController(request, action), journal_(journal) {}

#if defined(ARDUINO) && !defined(UNIT_TEST)

void DoseJournalController::registerRoutes(AsyncWebServer& server, IDoseJournal* journal) {
    server.on(AsyncURIMatcher::exact("/api/dosejournal"), HTTP_GET,
              [journal](AsyncWebServerRequest* request) { DoseJournalController(request, Action::Index, journal).dispatch(); });
    server.on(AsyncURIMatcher::exact("/api/dosejournal"), HTTP_OPTIONS,
              [journal](AsyncWebServerRequest* request) { DoseJournalController(request, Action::Options, journal).dispatch(); });
}

namespace {
// One journal entry copied out of the ring during the up-front snapshot. forEachNewestFirst is a
// push iterator that cannot be resumed, so we snapshot the (compact) fields we serialize and then
// drive the chunked producer by index. The snapshot is bounded to kMaxHistoryEntries and is far
// smaller than the JSON it produces.
struct DoseEntry {
    uint32_t epoch;
    uint8_t type;
    uint16_t amountCentiMl;
};

bool collectEntry(const DoseJournalRecordV1& record, void* contextPtr) {
    auto& entries = *static_cast<std::vector<DoseEntry>*>(contextPtr);
    if (entries.size() >= kMaxHistoryEntries) {
        return false;
    }
    entries.push_back(DoseEntry{record.epoch, record.type, record.amountCentiMl});
    return true;
}

struct DoseJournalProducerState {
    std::vector<DoseEntry> entries;
    size_t cursor = 0;
    int phase = 0; // 0 = prefix, 1 = entries, 2 = suffix, 3 = done
};

bool produceDoseJournalPiece(DoseJournalProducerState& s, BaseController::ChunkedBody& body) {
    if (s.phase == 0) {
        static const char kPrefix[] = "{\"success\":true,\"entries\":[";
        s.phase = 1;
        return body.emit(kPrefix, sizeof(kPrefix) - 1U);
    }

    if (s.phase == 1) {
        if (s.cursor < s.entries.size()) {
            const DoseEntry& e = s.entries[s.cursor];
            char piece[80];
            const int n = std::snprintf(piece, sizeof(piece), "%s{\"at\":%lu,\"type\":\"%s\",\"amountMl\":%.2f}", s.cursor == 0U ? "" : ",",
                                        static_cast<unsigned long>(e.epoch),
                                        e.type == static_cast<uint8_t>(DoseJournalEntryType::Schedule) ? "schedule" : "manual",
                                        static_cast<double>(e.amountCentiMl) / 100.0);
            ++s.cursor;
            return n > 0 && body.emit(piece, static_cast<size_t>(n));
        }
        s.phase = 2; // fall through to suffix
    }

    if (s.phase == 2) {
        s.phase = 3;
        return body.emit("]}", 2);
    }

    return false;
}
} // namespace

#endif

void DoseJournalController::index() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (journal_ == nullptr) {
        renderError(500, "INTERNAL", "dose journal is not available");
        return;
    }

    uint32_t deviceId = 0U;
    if (const AsyncWebParameter* deviceIdParam = request_->getParam("deviceId"); deviceIdParam != nullptr) {
        deviceId = static_cast<uint32_t>(deviceIdParam->value().toInt());
    }
    uint32_t periodDays = kDefaultPeriodDays;
    if (const AsyncWebParameter* periodParam = request_->getParam("periodDays"); periodParam != nullptr) {
        const long parsed = periodParam->value().toInt();
        if (parsed < 1L || parsed > static_cast<long>(kMaxPeriodDays)) {
            renderError(400, "BAD_ARGS", "periodDays must be 1-365");
            return;
        }
        periodDays = static_cast<uint32_t>(parsed);
    }

    // Journal epochs are local-flavored (DateTime::current().unixtime()); without a valid clock
    // there is no meaningful "last N days" cut, so return everything the ring still holds.
    uint32_t sinceEpoch = 0U;
    const DateTime nowWall = DateTime::current();
    if (nowWall.year() >= kDoseJournalMinValidYear) {
        const uint32_t periodSeconds = periodDays * 86400UL;
        sinceEpoch = nowWall.unixtime() > periodSeconds ? nowWall.unixtime() - periodSeconds : 0U;
    }

    auto state = std::make_shared<DoseJournalProducerState>();
    journal_->forEachNewestFirst(deviceId, sinceEpoch, &collectEntry, &state->entries);
    sendChunked("application/json", [state](ChunkedBody& body) { return produceDoseJournalPiece(*state, body); });
#endif
}

void DoseJournalController::options() {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    BaseController::options();
#endif
}

} // namespace ewfm
