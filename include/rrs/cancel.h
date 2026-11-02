#pragma once
#include <string>
#include <vector>

#include "rrs/types.h"

namespace rrs {

// Refund policy - SRS RRS-F-031. The slabs are named constants so they are easy to change.
//   more than 24 h before departure ..... 75 %
//   12 h to 24 h (inclusive) ............ 50 %
//   under 12 h (not yet departed) ....... 25 %
//   departed ............................ cancellation not allowed
// The same slabs apply to confirmed, RAC and waiting-list tickets (SRS 4.4).
constexpr long long kRefundOver24hMinutes = 24 * 60;  // strictly more than this -> first slab
constexpr long long kRefundFrom12hMinutes = 12 * 60;  // this or more (up to 24 h) -> second slab
constexpr int kRefundPercentOver24h = 75;
constexpr int kRefundPercent12To24h = 50;
constexpr int kRefundPercentUnder12h = 25;

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
