#include "test_helpers.h"

// Owner: Pranav - extra Cancel and Search tests (Sprint 1)

TEST(TC_Cancel_12_PnrWithSpacesIsAccepted) {
    Env e;
    auto b = bookN(e, "alice", "SL", 1);
    CHECK(e.cancel.cancel("  " + b.ticket.pnr + "  ", "alice").ok);
}
TEST(TC_Cancel_13_RequesterNameIsCaseInsensitive) {
    Env e;
    auto b = bookN(e, "alice", "SL", 1);
    CHECK(e.cancel.cancel(b.ticket.pnr, "ALICE").ok);
}
TEST(TC_Cancel_14_CancelledTicketFreesSeatsInSearch) {
    Env e;
    auto b = bookN(e, "alice", "SL", 4);
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].confirmedFree, 0);
    e.cancel.cancel(b.ticket.pnr, "alice");
    CHECK_EQ(e.search.search({"Bengaluru", "New Delhi", kDate, "SL"}).rows[0].confirmedFree, 4);
}
TEST(TC_Search_08_UnknownClassFilterGivesNoRows) {
    Env e;
    auto r = e.search.search({"Bengaluru", "New Delhi", kDate, "1A"});
    CHECK(r.ok);
    CHECK(r.rows.empty());
}
TEST(TC_Search_09_TodayIsAllowedAsJourneyDate) {
    Env e;  // now = 2026-10-01
    CHECK(e.search.search({"Bengaluru", "New Delhi", "2026-10-01", ""}).ok);
}
