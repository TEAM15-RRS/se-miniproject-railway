#include "rrs/search.h"

#include "rrs/inventory.h"
#include "rrs/util.h"

namespace rrs {

static std::string describe(const Seats& s) {
    if (s.confirmedFree > 0) return "AVAILABLE " + std::to_string(s.confirmedFree);
    if (s.racFree > 0) return "RAC " + std::to_string(s.racFree);
    if (s.wlFree > 0) return "WL " + std::to_string(s.wlFree);
    return "REGRET";
}

SearchResult SearchService::search(const SearchQuery& q) const {
    SearchResult r;
    std::string src = trim(q.source), dst = trim(q.destination), cls = trim(q.classCode);
    if (src.empty() || dst.empty()) { r.message = "Source and destination are required."; return r; }
    if (iequals(src, dst)) { r.message = "Source and destination must be different."; return r; }
    std::string dateErr = validateJourneyDate(q.journeyDate, clock_());
    if (!dateErr.empty()) { r.message = dateErr; return r; }

    for (const auto& t : db_.trains) {
        if (!t.active || !iequals(t.source, src) || !iequals(t.destination, dst)) continue;
        for (const auto& c : t.classes) {
            if (!cls.empty() && !iequals(c.code, cls)) continue;
            Seats s = computeSeats(db_, t.number, c, q.journeyDate);
            AvailabilityRow row;
            row.trainNumber = t.number; row.trainName = t.name; row.source = t.source;
            row.destination = t.destination; row.departure = t.departure; row.classCode = c.code;
            row.fare = c.fare; row.confirmedFree = s.confirmedFree; row.racFree = s.racFree;
            row.wlFree = s.wlFree; row.availability = describe(s);
            r.rows.push_back(row);
        }
    }
    r.ok = true;
    r.message = r.rows.empty() ? "No trains found for the selected route/class." : "OK";
    return r;
}

}  // namespace rrs
