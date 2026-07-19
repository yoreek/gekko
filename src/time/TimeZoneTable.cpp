#include "time/TimeZoneTable.h"

#include <cstring>

namespace ewfm {

// Human-readable labels live only in the SPA (test/test_core/timezones.json / portal-spa timezone
// catalog) - the firmware needs just the id + DST rules for validation and DateTime. The label is
// kept here as a trailing comment purely for readability and to document the id<->name mapping the
// UI relies on; it costs zero flash. Keep the id set/order in sync with timezones.json.
const TimeZoneEntry kTimeZoneTable[] = {
    {"Etc/GMT+12", {"STD", 1, 1, 1, 0, -720}, {"STD", 1, 1, 1, 0, -720}},                     // (GMT-12:00) Eniwetok, Kwajalein
    {"Pacific/Pago_Pago", {"STD", 1, 1, 1, 0, -660}, {"STD", 1, 1, 1, 0, -660}},              // (GMT-11:00) Midway Island, Samoa
    {"Pacific/Honolulu", {"STD", 1, 1, 1, 0, -600}, {"STD", 1, 1, 1, 0, -600}},               // (GMT-10:00) Hawaii
    {"America/Anchorage", {"AKDT", 2, 1, 3, 2, -480}, {"AKST", 1, 1, 11, 1, -540}},           // (GMT-09:00) Alaska
    {"America/Los_Angeles", {"PDT", 2, 1, 3, 2, -420}, {"PST", 1, 1, 11, 1, -480}},           // (GMT-08:00) Pacific Time (US, Canada)
    {"America/Denver", {"MDT", 2, 1, 3, 2, -360}, {"MST", 1, 1, 11, 1, -420}},                // (GMT-07:00) Mountain Time (US, Canada)
    {"America/Phoenix", {"STD", 1, 1, 1, 0, -420}, {"STD", 1, 1, 1, 0, -420}},                // (GMT-07:00) Arizona
    {"America/Mazatlan", {"MDT", 1, 1, 4, 2, -300}, {"MST", 0, 1, 10, 1, -360}},              // (GMT-07:00) Mazatlan
    {"America/Regina", {"STD", 1, 1, 1, 0, -360}, {"STD", 1, 1, 1, 0, -360}},                 // (GMT-06:00) Saskatchewan
    {"America/Mexico_City", {"CDT", 1, 1, 4, 2, -300}, {"CST", 0, 1, 10, 1, -360}},           // (GMT-06:00) Guadalajara, Mexico City
    {"America/Monterrey", {"CDT", 1, 1, 4, 2, -300}, {"CST", 0, 1, 10, 1, -360}},             // (GMT-06:00) Monterrey, Chihuahua
    {"America/Chicago", {"CDT", 2, 1, 3, 2, -300}, {"CST", 1, 1, 11, 1, -360}},               // (GMT-06:00) Central Time (US, Canada)
    {"America/New_York", {"EDT", 2, 1, 3, 2, -240}, {"EST", 1, 1, 11, 1, -300}},              // (GMT-05:00) Eastern Time (US, Canada)
    {"America/Indiana/Indianapolis", {"EDT", 2, 1, 3, 2, -240}, {"EST", 1, 1, 11, 1, -300}},  // (GMT-05:00) Indiana (East)
    {"America/Bogota", {"STD", 1, 1, 1, 0, -300}, {"STD", 1, 1, 1, 0, -300}},                 // (GMT-05:00) Bogota, Lima, Quito
    {"America/Halifax", {"ADT", 2, 1, 3, 2, -180}, {"AST", 1, 1, 11, 1, -240}},               // (GMT-04:00) Atlantic Time (Canada)
    {"America/La_Paz", {"STD", 1, 1, 1, 0, -240}, {"STD", 1, 1, 1, 0, -240}},                 // (GMT-04:00) La Paz
    {"America/Caracas", {"STD", 1, 1, 1, 0, -240}, {"STD", 1, 1, 1, 0, -240}},                // (GMT-04:00) Caracas
    {"America/Santiago", {"CLST", 1, 1, 9, 0, -180}, {"CLT", 1, 1, 4, 0, -240}},              // (GMT-04:00) Santiago
    {"America/Asuncion", {"PYDST", 1, 1, 10, 0, -180}, {"PYT", 2, 1, 3, 0, -240}},            // (GMT-04:00) Asuncion
    {"America/St_Johns", {"NDT", 2, 1, 3, 2, -150}, {"NST", 1, 1, 11, 1, -210}},              // (GMT-03:30) Newfoundland
    {"America/Sao_Paulo", {"STD", 1, 1, 1, 0, -180}, {"STD", 1, 1, 1, 0, -180}},              // (GMT-03:00) Brasilia
    {"America/Argentina/Buenos_Aires", {"STD", 1, 1, 1, 0, -180}, {"STD", 1, 1, 1, 0, -180}}, // (GMT-03:00) Buenos Aires, Georgetown
    {"America/Miquelon", {"STD", 1, 1, 1, 0, -180}, {"STD", 1, 1, 1, 0, -180}},               // (GMT-03:00) Saint Pierre, Miquelon
    {"America/Godthab", {"STD", 1, 1, 1, 0, -120}, {"STD", 1, 1, 1, 0, -120}},                // (GMT-02:00) Greenland
    {"Atlantic/South_Georgia", {"STD", 1, 1, 1, 0, -120}, {"STD", 1, 1, 1, 0, -120}},         // (GMT-02:00) South Georgia Islands
    {"Atlantic/Azores", {"WEST", 1, 1, 3, 1, -60}, {"WET", 0, 1, 10, 1, 0}},                  // (GMT-01:00) Azores
    {"Atlantic/Cape_Verde", {"STD", 1, 1, 1, 0, -60}, {"STD", 1, 1, 1, 0, -60}},              // (GMT-01:00) Cape Verde Is.
    {"Etc/GMT", {"STD", 1, 1, 1, 0, 0}, {"STD", 1, 1, 1, 0, 0}},                              // (GMT) Greenwich Mean Time
    {"Europe/London", {"BST", 0, 1, 3, 1, 60}, {"GMT", 0, 1, 10, 1, 0}},                      // (GMT) Dublin, Edinburgh, Lisbon, London
    {"Africa/Casablanca", {"STD", 1, 1, 1, 0, 0}, {"STD", 1, 1, 1, 0, 0}},                    // (GMT) Casablanca
    {"Africa/Monrovia", {"STD", 1, 1, 1, 0, 0}, {"STD", 1, 1, 1, 0, 0}},                      // (GMT) Monrovia
    {"Europe/Belgrade", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                 // (GMT+01:00) Belgrade, Bratislava, Budapest
    {"Europe/Prague", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                   // (GMT+01:00) Ljubljana, Prague
    {"Europe/Sarajevo", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                 // (GMT+01:00) Sarajevo, Skopje
    {"Europe/Warsaw", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                   // (GMT+01:00) Warsaw, Zagreb
    {"Europe/Copenhagen", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},               // (GMT+01:00) Copenhagen, Stockholm, Oslo
    {"Europe/Paris", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                    // (GMT+01:00) Madrid, Paris
    {"Europe/Berlin", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                   // (GMT+01:00) Amsterdam, Berlin, Brussels
    {"Europe/Rome", {"CEST", 0, 1, 3, 1, 120}, {"CET", 0, 1, 10, 1, 60}},                     // (GMT+01:00) Rome, Vienna, Bern
    {"Africa/Lagos", {"STD", 1, 1, 1, 0, 60}, {"STD", 1, 1, 1, 0, 60}},                       // (GMT+01:00) West Central Africa
    {"Europe/Vilnius", {"EEST", 0, 1, 3, 1, 180}, {"EET", 0, 1, 10, 1, 120}},                 // (GMT+02:00) Vilnius, Bucharest, Sofia
    {"Europe/Helsinki", {"EEST", 0, 1, 3, 1, 180}, {"EET", 0, 1, 10, 1, 120}},                // (GMT+02:00) Helsinki
    {"Africa/Cairo", {"STD", 1, 1, 1, 0, 120}, {"STD", 1, 1, 1, 0, 120}},                     // (GMT+02:00) Cairo
    {"Europe/Riga", {"EEST", 0, 1, 3, 1, 180}, {"EET", 0, 1, 10, 1, 120}},                    // (GMT+02:00) Riga, Tallinn
    {"Europe/Athens", {"EEST", 0, 1, 3, 1, 180}, {"EET", 0, 1, 10, 1, 120}},                  // (GMT+02:00) Athens
    {"Asia/Jerusalem", {"IDT", 0, 1, 3, 2, 180}, {"IST", 0, 1, 10, 2, 120}},                  // (GMT+02:00) Jerusalem
    {"Europe/Kyiv", {"EEST", 0, 1, 3, 1, 180}, {"EET", 0, 1, 10, 1, 120}},                    // (GMT+02:00) Kyiv
    {"Europe/Kaliningrad", {"STD", 1, 1, 1, 0, 120}, {"STD", 1, 1, 1, 0, 120}},               // (GMT+02:00) Kaliningrad
    {"Africa/Johannesburg", {"STD", 1, 1, 1, 0, 120}, {"STD", 1, 1, 1, 0, 120}},              // (GMT+02:00) Harare, Pretoria
    {"Asia/Riyadh", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                      // (GMT+03:00) Kuwait, Riyadh
    {"Africa/Nairobi", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                   // (GMT+03:00) Nairobi
    {"Europe/Minsk", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                     // (GMT+03:00) Minsk
    {"Europe/Moscow", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                    // (GMT+03:00) Moscow, St. Petersburg
    {"Europe/Volgograd", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                 // (GMT+03:00) Volgograd
    {"Asia/Baghdad", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                     // (GMT+03:00) Baghdad
    {"Europe/Istanbul", {"STD", 1, 1, 1, 0, 180}, {"STD", 1, 1, 1, 0, 180}},                  // (GMT+03:00) Istanbul
    {"Asia/Tehran", {"IRDT", 0, 5, 3, 24, 240}, {"IRST", 0, 5, 9, 24, 180}},                  // (GMT+03:30) Tehran
    {"Asia/Dubai", {"STD", 1, 1, 1, 0, 240}, {"STD", 1, 1, 1, 0, 240}},                       // (GMT+04:00) Abu Dhabi, Muscat
    {"Europe/Samara", {"STD", 1, 1, 1, 0, 240}, {"STD", 1, 1, 1, 0, 240}},                    // (GMT+04:00) Izhevsk, Samara
    {"Asia/Tbilisi", {"STD", 1, 1, 1, 0, 240}, {"STD", 1, 1, 1, 0, 240}},                     // (GMT+04:00) Tbilisi, Yerevan
    {"Asia/Baku", {"STD", 1, 1, 1, 0, 240}, {"STD", 1, 1, 1, 0, 240}},                        // (GMT+04:00) Baku
    {"Asia/Kabul", {"STD", 1, 1, 1, 0, 270}, {"STD", 1, 1, 1, 0, 270}},                       // (GMT+04:30) Kabul
    {"Asia/Karachi", {"STD", 1, 1, 1, 0, 300}, {"STD", 1, 1, 1, 0, 300}},                     // (GMT+05:00) Islamabad, Karachi, Tashkent
    {"Asia/Yekaterinburg", {"STD", 1, 1, 1, 0, 300}, {"STD", 1, 1, 1, 0, 300}},               // (GMT+05:00) Yekaterinburg
    {"Asia/Kolkata", {"IST", 1, 1, 1, 0, 330}, {"IST", 1, 1, 1, 0, 330}},          // (GMT+05:30) Kolkata, Chennai, Mumbai, New Delhi
    {"Asia/Colombo", {"IST", 1, 1, 1, 0, 330}, {"IST", 1, 1, 1, 0, 330}},          // (GMT+05:30) Sri Jayawardenepura
    {"Asia/Kathmandu", {"NPT", 1, 1, 1, 0, 345}, {"NPT", 1, 1, 1, 0, 345}},        // (GMT+05:45) Kathmandu
    {"Asia/Almaty", {"STD", 1, 1, 1, 0, 360}, {"STD", 1, 1, 1, 0, 360}},           // (GMT+06:00) Almaty
    {"Asia/Dhaka", {"STD", 1, 1, 1, 0, 360}, {"STD", 1, 1, 1, 0, 360}},            // (GMT+06:00) Astana, Dhaka
    {"Asia/Yangon", {"MMT", 1, 1, 1, 0, 390}, {"MMT", 1, 1, 1, 0, 390}},           // (GMT+06:30) Yangon
    {"Asia/Bangkok", {"STD", 1, 1, 1, 0, 420}, {"STD", 1, 1, 1, 0, 420}},          // (GMT+07:00) Bangkok, Hanoi, Jakarta
    {"Asia/Krasnoyarsk", {"STD", 1, 1, 1, 0, 420}, {"STD", 1, 1, 1, 0, 420}},      // (GMT+07:00) Krasnoyarsk
    {"Asia/Novosibirsk", {"STD", 1, 1, 1, 0, 420}, {"STD", 1, 1, 1, 0, 420}},      // (GMT+07:00) Novosibirsk
    {"Asia/Shanghai", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},         // (GMT+08:00) Beijing, Chongqing, Urumqi
    {"Asia/Hong_Kong", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},        // (GMT+08:00) Hong Kong
    {"Asia/Singapore", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},        // (GMT+08:00) Kuala Lumpur, Singapore
    {"Asia/Taipei", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},           // (GMT+08:00) Taipei
    {"Australia/Perth", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},       // (GMT+08:00) Perth
    {"Asia/Ulaanbaatar", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},      // (GMT+08:00) Ulaanbaatar
    {"Asia/Irkutsk", {"STD", 1, 1, 1, 0, 480}, {"STD", 1, 1, 1, 0, 480}},          // (GMT+08:00) Irkutsk
    {"Asia/Seoul", {"STD", 1, 1, 1, 0, 540}, {"STD", 1, 1, 1, 0, 540}},            // (GMT+09:00) Seoul
    {"Asia/Yakutsk", {"STD", 1, 1, 1, 0, 540}, {"STD", 1, 1, 1, 0, 540}},          // (GMT+09:00) Yakutsk
    {"Asia/Tokyo", {"STD", 1, 1, 1, 0, 540}, {"STD", 1, 1, 1, 0, 540}},            // (GMT+09:00) Osaka, Sapporo, Tokyo
    {"Australia/Darwin", {"STD", 1, 1, 1, 0, 570}, {"STD", 1, 1, 1, 0, 570}},      // (GMT+09:30) Darwin
    {"Australia/Adelaide", {"ACDT", 1, 1, 10, 2, 630}, {"ACST", 1, 1, 4, 3, 570}}, // (GMT+09:30) Adelaide
    {"Australia/Sydney", {"AEDT", 1, 1, 10, 2, 660}, {"AEST", 1, 1, 4, 3, 600}},   // (GMT+10:00) Canberra, Melbourne, Sydney
    {"Australia/Brisbane", {"STD", 1, 1, 1, 0, 600}, {"STD", 1, 1, 1, 0, 600}},    // (GMT+10:00) Brisbane
    {"Asia/Vladivostok", {"STD", 1, 1, 1, 0, 600}, {"STD", 1, 1, 1, 0, 600}},      // (GMT+10:00) Vladivostok
    {"Australia/Hobart", {"AEDT", 1, 1, 10, 2, 660}, {"AEST", 1, 1, 4, 3, 600}},   // (GMT+10:00) Hobart
    {"Pacific/Port_Moresby", {"STD", 1, 1, 1, 0, 600}, {"STD", 1, 1, 1, 0, 600}},  // (GMT+10:00) Guam, Port Moresby
    {"Pacific/Guadalcanal", {"STD", 1, 1, 1, 0, 660}, {"STD", 1, 1, 1, 0, 660}},   // (GMT+11:00) Solomon Is.
    {"Pacific/Noumea", {"STD", 1, 1, 1, 0, 660}, {"STD", 1, 1, 1, 0, 660}},        // (GMT+11:00) New Caledonia
    {"Asia/Srednekolymsk", {"STD", 1, 1, 1, 0, 660}, {"STD", 1, 1, 1, 0, 660}},    // (GMT+11:00) Chokurdakh, Srednekolymsk
    {"Asia/Magadan", {"STD", 1, 1, 1, 0, 660}, {"STD", 1, 1, 1, 0, 660}},          // (GMT+11:00) Magadan
    {"Pacific/Majuro", {"STD", 1, 1, 1, 0, 720}, {"STD", 1, 1, 1, 0, 720}},        // (GMT+12:00) Marshall Is.
    {"Pacific/Fiji", {"STD", 1, 1, 1, 0, 720}, {"STD", 1, 1, 1, 0, 720}},          // (GMT+12:00) Fiji
    {"Asia/Anadyr", {"STD", 1, 1, 1, 0, 720}, {"STD", 1, 1, 1, 0, 720}},           // (GMT+12:00) Anadyr, Petropavlovsk-Kamchatsky
    {"Pacific/Auckland", {"NZDT", 0, 1, 9, 2, 780}, {"NZST", 1, 1, 4, 3, 720}},    // (GMT+12:00) Auckland, Wellington
    {"Pacific/Tongatapu", {"STD", 1, 1, 1, 0, 780}, {"STD", 1, 1, 1, 0, 780}},     // (GMT+13:00) Nuku'alofa
};

const size_t kTimeZoneTableCount = sizeof(kTimeZoneTable) / sizeof(kTimeZoneTable[0]);

const TimeZoneEntry* findTimeZoneEntry(const char* id) {
    if (id == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < kTimeZoneTableCount; ++i) {
        if (std::strcmp(kTimeZoneTable[i].id, id) == 0) {
            return &kTimeZoneTable[i];
        }
    }
    return nullptr;
}

} // namespace ewfm
