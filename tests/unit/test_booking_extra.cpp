#include "test_helpers.h"

// Owner: Amruta - additional Booking tests (Sprint 1)
TEST(TC_Book_16_AdvanceWindowBoundary) {
    Env e;  // now = 2026-10-01
    CHECK(e.booking.book(req("alice", "SL", 1, kGoodCard, "2027-01-29")).ok);    // exactly 120 days ahead
    CHECK(!e.booking.book(req("alice", "SL", 1, kGoodCard, "2027-01-30")).ok);   // 121 days ahead
}
TEST(TC_Book_17_FareIsPerPassenger) {
    Env e;
    e.db.trains[0].classes[0].totalSeats = 6;
    auto r = bookN(e, "alice", "SL", 6);
    CHECK(r.ok);
    CHECK_EQ(r.ticket.totalFare, 3000);  // 6 x 500
}
TEST(TC_Book_18_ThirdAcRacWhenTwoBerthsTaken) {
    Env e;
    CHECK(bookN(e, "alice", "3A", 2).ticket.status == Status::Confirmed);
    CHECK(bookN(e, "bob", "3A", 1).ticket.status == Status::RAC);
    CHECK(bookN(e, "bob", "3A", 1).ticket.status == Status::Waitlist);
    CHECK(!bookN(e, "bob", "3A", 1).ok);
}
TEST(TC_Book_19_ClassCodeIsCaseInsensitiveAndStoredCanonical) {
    Env e;
    auto r = e.booking.book(req("alice", "sl", 1));
    CHECK(r.ok);
    CHECK_EQ(r.ticket.classCode, std::string("SL"));
}
TEST(TC_Book_20_PassengerNameIsTrimmed) {
    Env e;
    auto q = req("alice", "SL", 1);
    q.passengers[0].name = "   Ravi Kumar  ";
    auto r = e.booking.book(q);
    CHECK(r.ok);
    CHECK_EQ(r.ticket.passengers[0].name, std::string("Ravi Kumar"));
}
TEST(TC_Book_21_UsernameStoredWithOriginalCase) {
    Env e;
    auto r = e.booking.book(req("ALICE", "SL", 1));
    CHECK(r.ok);
    CHECK_EQ(r.ticket.username, std::string("alice"));
    CHECK_EQ(e.booking.ticketsFor("Alice").size(), size_t(1));
}
