#pragma once
#include <string>

namespace rrs {

struct PaymentResult {
    bool ok = false;
    std::string message;
    std::string txnId;
};

// Simulated payment gateway (SRS: no real bank integration). Deterministic:
//  - card must be 16 digits (spaces/dashes ignored) and pass the Luhn check
//  - 4000000000000002 is a Luhn-valid test card that is always declined
class PaymentService {
public:
    static std::string normalize(const std::string& card);
    static bool luhnValid(const std::string& digits);
    static std::string mask(const std::string& card);  // "************1111"
    PaymentResult charge(const std::string& card, int amount);

private:
    long long counter_ = 0;
};

}  // namespace rrs
