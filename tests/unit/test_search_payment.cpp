#include "test_helpers.h"

// ---- Search (RRS-F-010..012) ----
TEST(TC_Search_01_FindsTrainOnRoute) {
    Env e;
    auto r = e.search.search({"Bengaluru", "New Delhi", kDate, ""});
    CHECK(r.ok);
    CHECK_EQ(r.rows.size(), size_t(2));  // SL and 3A
    CHECK_EQ(r.rows[0].trainNumber, 12627);
    CHECK_EQ(r.rows[0].availability, std::string("AVAILABLE 4"));
}
TEST(TC_Search_02_CaseInsensitiveAndTrimmed) {
    Env e;
    CHECK_EQ(e.search.search({"  bengaluru ", "NEW delhi", kDate, "sl"}).rows.size(), size_t(1));
}
TEST(TC_Search_03_NoResultsIsNotAnError) {
    Env e;
    auto r = e.search.search({"Bengaluru", "Mysuru", kDate, ""});
    CHECK(r.ok);
    CHECK(r.rows.empty());
    CHECK_CONTAINS(r.message, "No trains");
}
TEST(TC_Search_04_InvalidInputs) {
    Env e;
    CHECK(!e.search.search({"", "New Delhi", kDate, ""}).ok);
    CHECK(!e.search.search({"Bengaluru", "", kDate, ""}).ok);
    CHECK(!e.search.search({"Bengaluru", "bengaluru", kDate, ""}).ok);       // same station
    CHECK(!e.search.search({"Bengaluru", "New Delhi", "2026-09-30", ""}).ok);  // past
    CHECK(!e.search.search({"Bengaluru", "New Delhi", "31/10/2026", ""}).ok);  // bad format
}
TEST(TC_Search_05_InactiveTrainHidden) {
    Env e;
    e.db.trains[0].active = false;
    CHECK(e.search.search({"Bengaluru", "New Delhi", kDate, ""}).rows.empty());
}
TEST(TC_Search_06_RacWlHintWhenClassFull) {  // RRS-F-012
    Env e;
    for (int i = 0; i < 4; ++i) bookN(e, "alice", "SL", 1);
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].availability, std::string("RAC 2"));
    for (int i = 0; i < 2; ++i) bookN(e, "alice", "SL", 1);
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].availability, std::string("WL 2"));
    for (int i = 0; i < 2; ++i) bookN(e, "alice", "SL", 1);
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].availability, std::string("REGRET"));
}
TEST(TC_Search_07_AvailabilityIsPerDate) {
    Env e;
    bookN(e, "alice", "SL", 4);
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", "2026-10-11", "SL"}).rows[0].availability,
             std::string("AVAILABLE 4"));
}

// ---- Payment (simulated) ----
TEST(UT_Pay_Luhn) {
    CHECK(PaymentService::luhnValid("4111111111111111"));
    CHECK(PaymentService::luhnValid("5555555555554444"));
    CHECK(!PaymentService::luhnValid("4111111111111112"));
    CHECK(!PaymentService::luhnValid(""));
    CHECK(!PaymentService::luhnValid("41111111x1111111"));
}
TEST(UT_Pay_ChargeSuccessAndFormats) {
    PaymentService p;
    CHECK(p.charge("4111 1111 1111 1111", 500).ok);
    CHECK(p.charge("4111-1111-1111-1111", 500).ok);
    auto a = p.charge("4111111111111111", 1), b = p.charge("4111111111111111", 1);
    CHECK(a.txnId != b.txnId);
}
TEST(UT_Pay_Rejections) {
    PaymentService p;
    CHECK(!p.charge("411111111111111", 100).ok);     // 15 digits
    CHECK(!p.charge("41111111111111111", 100).ok);   // 17 digits
    CHECK(!p.charge("abcdefghijklmnop", 100).ok);
    CHECK(!p.charge("4111111111111112", 100).ok);    // bad checksum
    CHECK(!p.charge("4111111111111111", 0).ok);
    CHECK(!p.charge("4111111111111111", -5).ok);
    auto d = p.charge(kDeclineCard, 100);
    CHECK(!d.ok);
    CHECK_CONTAINS(d.message, "declined");
}
TEST(TC_Sec_03_CardNumberMaskedForDisplay) {  // RRS-SR-002 (display helper)
    CHECK_EQ(PaymentService::mask("4111 1111 1111 1111"), std::string("************1111"));
}
