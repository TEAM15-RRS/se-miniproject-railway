#include "rrs/booking.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "rrs/inventory.h"
#include "rrs/util.h"

namespace rrs {

std::string pnrFromSeq(long long seq) { return std::to_string(1000000000LL + seq); }

std::string BookingService::validatePassenger(const Passenger& p) {
    std::string name = trim(p.name);
    if (name.empty() || name.size() > 30) return "Passenger name must be 1-30 characters.";
    for (char c : name)
        if (!std::isalpha(static_cast<unsigned char>(c)) && c != ' ' && c != '.' && c != '\'')
            return "Passenger name may contain only letters, spaces, '.' and apostrophe.";
    if (p.age < 1 || p.age > 120) return "Passenger age must be between 1 and 120.";
    if (p.gender != 'M' && p.gender != 'F' && p.gender != 'O') return "Gender must be M, F or O.";
    return "";
}

BookingResult BookingService::book(const BookingRequest& req) {
    BookingResult res;
    auto fail = [&](const std::string& m) { res.ok = false; res.message = m; return res; };

    if (!findUser(db_, req.username)) return fail("User not found. Please log in.");
    const Train* train = findTrain(db_, req.trainNumber);
    if (!train || !train->active) return fail("Train not found or not currently running.");
    const ClassInfo* cls = train->findClass(req.classCode);
    if (!cls) return fail("Selected class is not available on this train.");

    const long long now = clock_();
    std::string dateErr = validateJourneyDate(req.journeyDate, now);
    if (!dateErr.empty()) return fail(dateErr);
    long long departure = 0;
    if (!toEpochMinutes(req.journeyDate, train->departure, departure)) return fail("Train timetable is invalid.");
    if (departure <= now) return fail("This train has already departed.");

    const int n = static_cast<int>(req.passengers.size());
    if (n < 1 || n > kMaxPassengersPerBooking)
        return fail("A booking must have 1 to " + std::to_string(kMaxPassengersPerBooking) + " passengers.");
    for (size_t i = 0; i < req.passengers.size(); ++i) {
        std::string e = validatePassenger(req.passengers[i]);
        if (!e.empty()) return fail("Passenger " + std::to_string(i + 1) + ": " + e);
    }

    Seats s = computeSeats(db_, train->number, *cls, req.journeyDate);
    Status status;
    if (s.confirmedFree >= n) status = Status::Confirmed;
    else if (s.racFree >= n) status = Status::RAC;
    else if (s.wlFree >= n) status = Status::Waitlist;
    else return fail("No seats, RAC or waiting-list capacity left for this class on that date.");

    const int fare = cls->fare * n;
    PaymentResult pay = pay_.charge(req.cardNumber, fare);
    if (!pay.ok) return fail(pay.message);  // nothing is stored if payment fails

    Ticket t;
    t.seq = db_.nextSeq();
    t.pnr = pnrFromSeq(t.seq);
    t.username = findUser(db_, req.username)->username;
    t.trainNumber = train->number;
    t.classCode = cls->code;
    t.journeyDate = req.journeyDate;
    t.passengers = req.passengers;
    for (auto& p : t.passengers) p.name = trim(p.name);
    t.status = status;
    t.totalFare = fare;
    t.bookedAt = now;
    db_.tickets.push_back(t);

    res.ok = true;
    res.ticket = t;
    res.txnId = pay.txnId;
    res.message = "Booking " + toString(status) + ". PNR " + t.pnr;
    return res;
}

std::vector<const Ticket*> BookingService::ticketsFor(const std::string& username) const {
    std::vector<const Ticket*> out;
    for (const auto& t : db_.tickets)
        if (iequals(t.username, username)) out.push_back(&t);
    std::sort(out.begin(), out.end(), [](const Ticket* a, const Ticket* b) { return a->seq > b->seq; });
    return out;
}

const Ticket* lookupPnr(const Database& db, const std::string& pnr) { return findTicket(db, trim(pnr)); }

std::string formatReceipt(const Database& db, const Ticket& t, const std::string& txnId) {
    const Train* tr = findTrain(db, t.trainNumber);
    std::ostringstream o;
    o << "========== E-TICKET ==========\n"
      << "PNR       : " << t.pnr << "\n"
      << "Train     : " << t.trainNumber << " " << (tr ? tr->name : "?") << "\n";
    if (tr) o << "Route     : " << tr->source << " -> " << tr->destination << "  Dep " << tr->departure << "\n";
    o << "Date      : " << t.journeyDate << "   Class: " << t.classCode << "\n"
      << "Status    : " << toString(t.status);
    int pos = queuePosition(db, t);
    if (pos > 0) o << " (position " << pos << ")";
    o << "\nPassengers:\n";
    for (size_t i = 0; i < t.passengers.size(); ++i)
        o << "  " << (i + 1) << ". " << t.passengers[i].name << ", " << t.passengers[i].age << ", "
          << t.passengers[i].gender << "\n";
    o << "Total fare: Rs." << t.totalFare << "\n";
    if (!txnId.empty()) o << "Payment   : " << txnId << "\n";
    if (t.status == Status::Cancelled) o << "Refund    : Rs." << t.refundAmount << "\n";
    o << "==============================\n";
    return o.str();
}

}  // namespace rrs
