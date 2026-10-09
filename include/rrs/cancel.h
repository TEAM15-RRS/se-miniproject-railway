#pragma once
#include <string>
#include <vector>

#include "rrs/types.h"

namespace rrs {

// Refund policy (assumption A-3 in docs/SPRINT1_PLAN.md - align with SRS section on refunds):
//   waiting-list ticket ........................ 100 % of fare
//   >= 48 h before departure ................... 100 % minus Rs.60 per passenger (min 0)
//   >= 12 h and < 48 h ......................... 75 %
//   >=  4 h and < 12 h ......................... 50 %
//   <   4 h (but not yet departed) ............. 0 %
//   departed ................................... cancellation not allowed
constexpr int kFlatCancellationFee = 60;

int calculateRefund(const Ticket& t, long long departureMinutes, long long nowMinutes);

struct CancelResult {
    bool ok = false;
    std::string message;
    int refund = 0;
    std::vector<std::string> promotedPnrs;  // tickets whose status improved (RRS-F-032)
};

class CancelService {
public:
    CancelService(Database& db, Clock clock) : db_(db), clock_(std::move(clock)) {}

    CancelResult cancel(const std::string& pnr, const std::string& requester);  // RRS-F-030, F-031, F-033

    // FIFO promotion: RAC -> CONFIRMED, then WL -> RAC/CONFIRMED. Returns changed PNRs.
    std::vector<std::string> promote(int trainNumber, const std::string& classCode, const std::string& date);

private:
    Database& db_;
    Clock clock_;
};

}  // namespace rrs
