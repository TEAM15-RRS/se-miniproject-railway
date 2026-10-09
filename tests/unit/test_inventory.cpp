#include "test_helpers.h"

// Owner: Amruta - Inventory module (seat counting and queue position)

static const ClassInfo& sl(Env& e) { return *e.db.trains[0].findClass("SL"); }

TEST(TC_Inv_01_EmptyTrainHasAllSeatsFree) {
    Env e;
    Seats s = computeSeats(e.db, 12627, sl(e), kDate);
    CHECK_EQ(s.confirmedFree, 4);
    CHECK_EQ(s.racFree, 2);
    CHECK_EQ(s.wlFree, 2);
}
TEST(TC_Inv_02_CancelledTicketsDoNotUseSeats) {
    Env e;
    auto b = bookN(e, "alice", "SL", 3);
    e.cancel.cancel(b.ticket.pnr, "alice");
    Seats s = computeSeats(e.db, 12627, sl(e), kDate);
    CHECK_EQ(s.confirmedUsed, 0);
    CHECK_EQ(s.confirmedFree, 4);
}
TEST(TC_Inv_03_CountsAreSeparatePerClassAndDate) {
    Env e;
    bookN(e, "alice", "SL", 2);
    CHECK_EQ(computeSeats(e.db, 12627, *e.db.trains[0].findClass("3A"), kDate).confirmedUsed, 0);
    CHECK_EQ(computeSeats(e.db, 12627, sl(e), "2026-10-11").confirmedUsed, 0);
    CHECK_EQ(computeSeats(e.db, 12627, sl(e), kDate).confirmedUsed, 2);
}
TEST(TC_Inv_04_PassengersCountedNotTickets) {
    Env e;
    bookN(e, "alice", "SL", 3);
    bookN(e, "bob", "SL", 1);
    CHECK_EQ(computeSeats(e.db, 12627, sl(e), kDate).confirmedUsed, 4);
}
TEST(TC_Inv_05_QueuePositionOnlyForRacAndWaitlist) {
    Env e;
    auto c = bookN(e, "alice", "SL", 1);
    CHECK_EQ(queuePosition(e.db, *lookupPnr(e.db, c.ticket.pnr)), 0);  // confirmed -> no queue
}
TEST(TC_Inv_06_QueuePositionCountsPassengersAhead) {
    Env e;
    for (int i = 0; i < 4; ++i) bookN(e, "bob", "SL", 1);  // confirmed full
    auto r1 = bookN(e, "alice", "SL", 1);                  // RAC, position 1
    auto r2 = bookN(e, "alice", "SL", 1);                  // RAC, position 2
    CHECK_EQ(queuePosition(e.db, *lookupPnr(e.db, r1.ticket.pnr)), 1);
    CHECK_EQ(queuePosition(e.db, *lookupPnr(e.db, r2.ticket.pnr)), 2);
}
