#include "rrs/inventory.h"

namespace rrs {

static bool sameGroup(const Ticket& t, int trainNumber, const std::string& cls, const std::string& date) {
    return t.trainNumber == trainNumber && t.classCode == cls && t.journeyDate == date;
}

Seats computeSeats(const Database& db, int trainNumber, const ClassInfo& cls, const std::string& date) {
    Seats s;
    for (const auto& t : db.tickets) {
        if (!sameGroup(t, trainNumber, cls.code, date)) continue;
        int n = static_cast<int>(t.passengers.size());
        if (t.status == Status::Confirmed) s.confirmedUsed += n;
        else if (t.status == Status::RAC) s.racUsed += n;
        else if (t.status == Status::Waitlist) s.wlUsed += n;
    }
    s.confirmedFree = cls.totalSeats - s.confirmedUsed;
    s.racFree = cls.racSeats - s.racUsed;
    s.wlFree = cls.wlLimit - s.wlUsed;
    return s;
}

int queuePosition(const Database& db, const Ticket& t) {
    if (t.status != Status::RAC && t.status != Status::Waitlist) return 0;
    int pos = 1;
    for (const auto& o : db.tickets) {
        if (o.status == t.status && o.trainNumber == t.trainNumber && o.classCode == t.classCode &&
            o.journeyDate == t.journeyDate && o.seq < t.seq)
            pos += static_cast<int>(o.passengers.size());
    }
    return pos;
}

}  // namespace rrs
