#include "rrs/payment.h"

#include <cctype>

namespace rrs {

std::string PaymentService::normalize(const std::string& card) {
    std::string out;
    for (char c : card)
        if (c != ' ' && c != '-') out.push_back(c);
    return out;
}

bool PaymentService::luhnValid(const std::string& d) {
    if (d.empty()) return false;
    int sum = 0;
    bool dbl = false;
    for (size_t i = d.size(); i-- > 0;) {
        if (!std::isdigit(static_cast<unsigned char>(d[i]))) return false;
        int v = d[i] - '0';
        if (dbl) { v *= 2; if (v > 9) v -= 9; }
        sum += v;
        dbl = !dbl;
    }
    return sum % 10 == 0;
}

std::string PaymentService::mask(const std::string& card) {
    std::string n = normalize(card);
    if (n.size() <= 4) return std::string(n.size(), '*');
    return std::string(n.size() - 4, '*') + n.substr(n.size() - 4);
}

PaymentResult PaymentService::charge(const std::string& card, int amount) {
    if (amount <= 0) return {false, "Invalid payment amount.", ""};
    std::string n = normalize(card);
    if (n.size() != 16) return {false, "Card number must be 16 digits.", ""};
    for (char c : n)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return {false, "Card number must contain digits only.", ""};
    if (!luhnValid(n)) return {false, "Invalid card number.", ""};
    if (n == "4000000000000002") return {false, "Payment declined by bank (simulated).", ""};
    return {true, "Payment successful.", "TXN" + std::to_string(100000 + (++counter_))};
}

}  // namespace rrs
