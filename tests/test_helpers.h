#pragma once
// Shared fixture: one tiny train (12627, SL: 4 berths + 2 RAC + 2 WL, 3A: 2 + 1 + 1) and a controllable clock.
#include <filesystem>

#include "rrs/auth.h"
#include "rrs/booking.h"
#include "rrs/cancel.h"
#include "rrs/inventory.h"
#include "rrs/search.h"
#include "rrs/storage.h"
#include "rrs/util.h"
#include "test_framework.h"

using namespace rrs;

inline const char* kDate = "2026-10-10";          // journey date used by most tests
inline const char* kGoodCard = "4111111111111111";  // Luhn-valid
inline const char* kDeclineCard = "4000000000000002";

inline long long M(const std::string& date, const std::string& time) {
    long long v = 0;
    toEpochMinutes(date, time, v);
    return v;
}

struct Env {
    Database db;
    long long now;
    Clock clock;
    AuthService auth;
    PaymentService pay;
    BookingService booking;
    CancelService cancel;
    SearchService search;

    Env()
        : now(M("2026-10-01", "10:00")), clock([this] { return now; }), auth(db, clock),
          booking(db, pay, clock), cancel(db, clock), search(db, clock) {
        Train t;
        t.number = 12627; t.name = "Karnataka Express"; t.source = "Bengaluru"; t.destination = "New Delhi";
        t.departure = "19:20";
        t.classes = {{"SL", 4, 2, 2, 500}, {"3A", 2, 1, 1, 1000}};
        db.trains.push_back(t);
        auth.registerUser("alice", "Passw0rd1");
        auth.registerUser("bob", "Passw0rd2");
    }
};

inline Passenger P(const std::string& name = "Ravi Kumar", int age = 30, char g = 'M') { return {name, age, g}; }

inline BookingRequest req(const std::string& user, const std::string& cls, int nPassengers,
                          const std::string& card = kGoodCard, const std::string& date = kDate) {
    BookingRequest r;
    r.username = user; r.trainNumber = 12627; r.classCode = cls; r.journeyDate = date; r.cardNumber = card;
    for (int i = 0; i < nPassengers; ++i) r.passengers.push_back(P("Passenger " + std::string(1, char('A' + i))));
    return r;
}

inline BookingResult bookN(Env& e, const std::string& user, const std::string& cls, int n) {
    return e.booking.book(req(user, cls, n));
}

inline std::string freshTempDir(const std::string& tag) {
    static int counter = 0;
    auto p = std::filesystem::temp_directory_path() / ("rrs_test_" + tag + "_" + std::to_string(++counter));
    std::filesystem::remove_all(p);
    std::filesystem::create_directories(p);
    return p.string();
}
