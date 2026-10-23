#pragma once
#include <string>
#include <vector>

#include "rrs/payment.h"
#include "rrs/types.h"

namespace rrs {

constexpr int kMaxPassengersPerBooking = 6;

struct BookingRequest {
    std::string username;
    int trainNumber = 0;
    std::string classCode;
    std::string journeyDate;
    std::vector<Passenger> passengers;
    std::string cardNumber;  // dummy card for simulated payment
};

struct BookingResult {
    bool ok = false;
    std::string message;
    Ticket ticket;
    std::string txnId;
};

class BookingService {
public:
    BookingService(Database& db, PaymentService& pay, Clock clock)
        : db_(db), pay_(pay), clock_(std::move(clock)) {}

    // RRS-F-020..024. All passengers on one PNR share one status (all-or-nothing):
    // CONFIRMED if enough berths, else RAC if enough RAC slots, else WL, else rejected.
    BookingResult book(const BookingRequest& req);

    std::vector<const Ticket*> ticketsFor(const std::string& username) const;  // newest first
    static std::string validatePassenger(const Passenger& p);                  // "" = valid

private:
    Database& db_;
    PaymentService& pay_;
    Clock clock_;
};

// PNR status lookup (RRS-F-034); returns nullptr when unknown.
const Ticket* lookupPnr(const Database& db, const std::string& pnr);
std::string formatReceipt(const Database& db, const Ticket& t, const std::string& txnId = "",
                          const std::string& maskedCard = "");  // RRS-F-025
std::string pnrFromSeq(long long seq);

}  // namespace rrs