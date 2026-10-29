#include "test_helpers.h"

// Owner: Amruta - Booking tests, round 3 (Sprint 1)

TEST(TC_Book_22_BookingOnJourneyDayBeforeDeparture) {
    Env e;
    e.now = M(kDate, "10:00");            // journey day, train leaves 19:20
    auto r = bookN(e, "alice", "SL", 1);
    CHECK(r.ok);
    CHECK(r.ticket.status == Status::Confirmed);
}
TEST(TC_Book_23_DifferentDatesHaveIndependentSeats) {
    Env e;
    CHECK(bookN(e, "alice", "SL", 4).ticket.status == Status::Confirmed);       // date 1 full
    auto other = e.booking.book(req("bob", "SL", 4, kGoodCard, "2026-10-11"));  // date 2
    CHECK(other.ok);
    CHECK(other.ticket.status == Status::Confirmed);
}
TEST(TC_Book_24_SingleFareMatchesClassFare) {
    Env e;
    CHECK_EQ(bookN(e, "alice", "SL", 1).ticket.totalFare, 500);
    CHECK_EQ(bookN(e, "alice", "3A", 1).ticket.totalFare, 1000);
}
TEST(TC_Book_25_BookedAtTimeIsRecorded) {
    Env e;
    auto r = bookN(e, "alice", "SL", 1);
    CHECK_EQ(r.ticket.bookedAt, e.now);
}
