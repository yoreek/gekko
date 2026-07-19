#pragma once

#include "devices/dosing/journal/DoseJournal.h"
#include "portal/controllers/BaseController.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
class AsyncWebServer;
class AsyncWebServerRequest;
#endif

namespace ewfm {

// GET /api/dosejournal?deviceId=<id>&periodDays=<n> - newest-first dose history from the
// persistent journal (deviceId 0/absent = all pumps). Entries stream straight from the journal
// segments into the response; stats (totals, daily average) are computed client-side.
class DoseJournalController : public BaseController {
public:
    DoseJournalController(AsyncWebServerRequest* request, Action action, IDoseJournal* journal);

#if defined(ARDUINO) && !defined(UNIT_TEST)
    static void registerRoutes(AsyncWebServer& server, IDoseJournal* journal);
#endif

protected:
    void index() override;
    void options() override;

private:
    IDoseJournal* journal_;
};

} // namespace ewfm
