#pragma once
#include <string>

#include "rrs/types.h"

namespace rrs {

// Seat counts for one (train, class, date). Derived from tickets, so it can never drift
// out of sync with the stored bookings.
struct Seats {
    int confirmedUsed = 0, racUsed = 0, wlUsed = 0;
    int confirmedFree = 0, racFree = 0, wlFree = 0;
};

Seats computeSeats(const Database& db, int trainNumber, const ClassInfo& cls, const std::string& date);

// 1-based position of a RAC/WL ticket inside its queue (counted in passengers). 0 otherwise.
int queuePosition(const Database& db, const Ticket& t);

}  // namespace rrs
