#include "test_helpers.h"

// Owner: Amruta - Payment tests (Sprint 1)

TEST(TC_Pay_01_CardWithLettersIsRejected) {
    PaymentService p;
    auto r = p.charge("4111abcd11111111", 100);   // 16 characters, but not all digits
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "digits");
}
TEST(TC_Pay_02_EveryChargeGetsADifferentTransactionId) {
    PaymentService p;
    auto a = p.charge(kGoodCard, 100);
    auto b = p.charge(kGoodCard, 100);
    auto c = p.charge(kGoodCard, 100);
    CHECK(a.ok && b.ok && c.ok);
    CHECK(a.txnId != b.txnId);
    CHECK(b.txnId != c.txnId);
    CHECK(a.txnId != c.txnId);
}
TEST(TC_Pay_03_AmountBoundaries) {
    PaymentService p;
    CHECK(!p.charge(kGoodCard, -1).ok);
    CHECK(!p.charge(kGoodCard, 0).ok);
    CHECK(p.charge(kGoodCard, 1).ok);          // smallest valid amount
    CHECK(p.charge(kGoodCard, 1000000).ok);    // large amount is fine
}
TEST(TC_Pay_04_SpacesAndDashesAreRemovedFromCardNumber) {
    CHECK_EQ(PaymentService::normalize("4111 1111-1111 1111"), std::string("4111111111111111"));
}
TEST(TC_Pay_05_DeclinedCardGivesNoTransactionId) {
    PaymentService p;
    auto r = p.charge(kDeclineCard, 500);
    CHECK(!r.ok);
    CHECK(r.txnId.empty());
}
TEST(TC_Pay_06_MaskHidesAllButLastFourDigits) {
    CHECK_EQ(PaymentService::mask("123"), std::string("***"));
    CHECK_EQ(PaymentService::mask("1234"), std::string("****"));
    CHECK_EQ(PaymentService::mask("12345"), std::string("*2345"));
    CHECK_EQ(PaymentService::mask(kGoodCard), std::string("************1111"));
}
TEST(TC_Pay_07_RealTestCardsPassLuhn) {
    CHECK(PaymentService::luhnValid("4012888888881881"));    // Visa test number
    CHECK(PaymentService::luhnValid("5555555555554444"));    // Mastercard test number
}
TEST(TC_Pay_08_FifteenDigitCardIsRejectedEvenIfLuhnValid) {
    PaymentService p;
    CHECK(PaymentService::luhnValid("378282246310005"));     // 15-digit Amex test number
    auto r = p.charge("378282246310005", 100);
    CHECK(!r.ok);
    CHECK_CONTAINS(r.message, "16 digits");
}
