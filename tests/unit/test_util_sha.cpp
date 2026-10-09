#include "rrs/sha256.h"
#include "test_helpers.h"

TEST(UT_Util_Sha256_KnownVectors) {
    CHECK_EQ(sha256Hex(""), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    CHECK_EQ(sha256Hex("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}
TEST(UT_Util_Sha256_MultiBlockInput) {
    CHECK_EQ(sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
             std::string("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
}
TEST(UT_Util_ParseDate_Valid) {
    int y, m, d;
    CHECK(parseDate("2026-10-10", y, m, d));
    CHECK(y == 2026 && m == 10 && d == 10);
}
TEST(UT_Util_ParseDate_LeapYearBoundaries) {
    int y, m, d;
    CHECK(parseDate("2028-02-29", y, m, d));   // leap year
    CHECK(!parseDate("2027-02-29", y, m, d));  // not leap
    CHECK(!parseDate("2100-02-29", y, m, d));  // century, not leap
}
TEST(UT_Util_ParseDate_RejectsMalformed) {
    int y, m, d;
    for (const char* s : {"", "2026-13-01", "2026-00-10", "2026-04-31", "10-10-2026", "2026/10/10",
                          "2026-1-1", "abcd-ef-gh", "1999-12-31", "2026-10-10 "})
        CHECK(!parseDate(s, y, m, d));
}
TEST(UT_Util_ParseTime) {
    int h, m;
    CHECK(parseTime("00:00", h, m));
    CHECK(parseTime("23:59", h, m));
    CHECK(!parseTime("24:00", h, m));
    CHECK(!parseTime("12:60", h, m));
    CHECK(!parseTime("9:30", h, m));
}
TEST(UT_Util_EpochMinutes_Difference) {
    CHECK_EQ(M("2026-10-02", "00:00") - M("2026-10-01", "00:00"), 1440LL);
    CHECK_EQ(M("2026-10-01", "10:30") - M("2026-10-01", "10:00"), 30LL);
}
TEST(UT_Util_ValidateJourneyDate_Window) {
    long long now = M("2026-10-01", "10:00");
    CHECK_EQ(validateJourneyDate("2026-10-01", now), std::string(""));   // today OK
    CHECK_CONTAINS(validateJourneyDate("2026-09-30", now), "past");
    CHECK_EQ(validateJourneyDate("2027-01-29", now), std::string(""));   // exactly 120 days
    CHECK_CONTAINS(validateJourneyDate("2027-01-30", now), "120");       // 121 days
    CHECK_CONTAINS(validateJourneyDate("not-a-date", now), "Invalid");
}
TEST(UT_Util_Split_KeepsEmptyFields) {
    auto v = split("a||b|", '|');
    CHECK_EQ(v.size(), size_t(4));
    CHECK_EQ(v[1], std::string(""));
}
