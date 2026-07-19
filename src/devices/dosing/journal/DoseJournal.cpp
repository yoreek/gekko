#include "devices/dosing/journal/DoseJournal.h"

namespace ewfm {

namespace {
IDoseJournal* g_defaultDoseJournal = nullptr;
} // namespace

IDoseJournal* defaultDoseJournal() {
    return g_defaultDoseJournal;
}

void setDefaultDoseJournal(IDoseJournal* journal) {
    g_defaultDoseJournal = journal;
}

} // namespace ewfm
