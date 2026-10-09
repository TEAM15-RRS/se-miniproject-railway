#include "rrs/cancel.h"

#include <algorithm>

#include "rrs/inventory.h"
#include "rrs/util.h"

namespace rrs {

int calculateRefund(const Ticket& t, long long departureMinutes, long long nowMinutes) {
    if (t.status == Status::Waitlist) return t.totalFare;
    const long long minutesLeft = departureMinutes - nowMinutes;
    const int n = static_cast<int>(t.passengers.size());
    if (minutesLeft >= 48 * 60) return std::max(0, t.totalFare - kFlatCancellationFee * n);
    if (minutesLeft >= 12 * 60) return t.totalFare * 75 / 100;
    if (minutesLeft >= 4 * 60) return t.totalFare * 50 / 100;
    return 0;
}

CancelResult CancelService::cancel(const std::string& pnr, const std::string& requester) {
    CancelResult r;
    Ticket* t = findTicket(db_, trim(pnr));
    if (!t) { r.message = "PNR not found."; return r; }
    if (!iequals(t->username, requester)) { r.message = "You can cancel only your own tickets."; return r; }
    if (t->status == Status::Cancelled) { r.message = "Ticket is already cancelled."; return r; }

    const Train* train = findTrain(db_, t->trainNumber);
    long long departure = 0;
    if (!train || !toEpochMinutes(t->journeyDate, train->departure, departure)) {
        r.message = "Train details unavailable for this ticket.";
        return r;
    }
    const long long now = clock_();
    if (departure <= now) { r.message = "The train has already departed; cancellation is not possible."; return r; }

    r.refund = calculateRefund(*t, departure, now);
    t->refundAmount = r.refund;
    t->cancelledAt = now;
    t->status = Status::Cancelled;  // record is retained (RRS-F-033)

    r.promotedPnrs = promote(t->trainNumber, t->classCode, t->journeyDate);
    r.ok = true;
    r.message = "Ticket " + t->pnr + " cancelled. Refund: Rs." + std::to_string(r.refund);
    return r;
}

std::vector<std::string> CancelService::promote(int trainNumber, const std::string& classCode,
                                                const std::string& date) {
    std::vector<std::string> changed;
    const Train* train = findTrain(db_, trainNumber);
    const ClassInfo* cls = train ? train->findClass(classCode) : nullptr;
    if (!cls) return changed;

    std::vector<Ticket*> queue;  // RAC and WL tickets of this group, booking order
    for (auto& t : db_.tickets)
        if (t.trainNumber == trainNumber && t.classCode == cls->code && t.journeyDate == date &&
            (t.status == Status::RAC || t.status == Status::Waitlist))
            queue.push_back(&t);
    std::sort(queue.begin(), queue.end(), [](Ticket* a, Ticket* b) { return a->seq < b->seq; });

    auto note = [&](const Ticket* t) {
        if (std::find(changed.begin(), changed.end(), t->pnr) == changed.end()) changed.push_back(t->pnr);
    };

    bool progress = true;
    while (progress) {
        progress = false;
        // 1) RAC -> CONFIRMED, strictly first-come-first-served
        for (Ticket* t : queue) {
            if (t->status != Status::RAC) continue;
            Seats s = computeSeats(db_, trainNumber, *cls, date);
            if (s.confirmedFree >= static_cast<int>(t->passengers.size())) {
                t->status = Status::Confirmed; note(t); progress = true;
            } else break;  // do not let later RAC tickets jump the queue
        }
        // 2) WL -> RAC / CONFIRMED. A WL ticket may take a confirmed berth only if no RAC is waiting.
        for (Ticket* t : queue) {
            if (t->status != Status::Waitlist) continue;
            bool racPending = std::any_of(queue.begin(), queue.end(),
                                          [](Ticket* x) { return x->status == Status::RAC; });
            Seats s = computeSeats(db_, trainNumber, *cls, date);
            int n = static_cast<int>(t->passengers.size());
            if (!racPending && s.confirmedFree >= n) { t->status = Status::Confirmed; }
            else if (s.racFree >= n) { t->status = Status::RAC; }
            else break;
            note(t); progress = true;
        }
    }
    return changed;
}

}  // namespace rrs
