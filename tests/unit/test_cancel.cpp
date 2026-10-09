#include "test_helpers.h"

// Journey 2026-10-10, departure 19:20. Helper: set clock to N minutes before departure.
static void setBefore(Env& e, long long minutesBefore) { e.now = M(kDate, "19:20") - minutesBefore; }

TEST(TC_Cancel_01_BasicCancelRetainsRecord) {  // RRS-F-030, F-033
    Env e;
    auto b = bookN(e, "alice", "SL", 1);
    auto r = e.cancel.cancel(b.ticket.pnr, "alice");
    CHECK(r.ok);
    const Ticket* t = lookupPnr(e.db, b.ticket.pnr);
    CHECK(t != nullptr);
    CHECK(t->status == Status::Cancelled);
    CHECK_EQ(e.db.tickets.size(), size_t(1));  // not deleted
    CHECK(t->cancelledAt == e.now);
}
TEST(TC_Cancel_02_RefundSlabBoundaries) {  // RRS-F-031
    // 2 passengers, fare 1000 total
    struct Row { long long before; int expected; };
    Row rows[] = {
        {48 * 60,       880},  // exactly 48h -> 100% - 60*2
        {48 * 60 - 1,   750},  // 47h59 -> 75%
        {12 * 60,       750},  // exactly 12h -> 75%
        {12 * 60 - 1,   500},  // 11h59 -> 50%
        {4 * 60,        500},  // exactly 4h -> 50%
        {4 * 60 - 1,      0},  // 3h59 -> 0
        {1,               0},  // 1 minute before departure
    };
    for (const auto& row : rows) {
        Env e;
        auto b = bookN(e, "alice", "SL", 2);
        setBefore(e, row.before);
        auto r = e.cancel.cancel(b.ticket.pnr, "alice");
        CHECK(r.ok);
        CHECK_EQ(r.refund, row.expected);
        CHECK_EQ(lookupPnr(e.db, b.ticket.pnr)->refundAmount, row.expected);
    }
}
TEST(TC_Cancel_03_RefundNeverNegative) {
    Env e;
    e.db.trains[0].classes[0].fare = 20;   // 2 pax -> 40 total < 120 flat fee
    auto b = bookN(e, "alice", "SL", 2);
    auto r = e.cancel.cancel(b.ticket.pnr, "alice");
    CHECK_EQ(r.refund, 0);
}
TEST(TC_Cancel_04_WaitlistTicketFullRefund) {
    Env e;
    for (int i = 0; i < 6; ++i) bookN(e, "alice", "SL", 1);
    auto w = bookN(e, "bob", "SL", 1);
    CHECK(w.ticket.status == Status::Waitlist);
    setBefore(e, 60);  // 1 hour before departure
    CHECK_EQ(e.cancel.cancel(w.ticket.pnr, "bob").refund, 500);
}
TEST(TC_Cancel_05_RejectsUnknownAlreadyCancelledWrongOwnerDeparted) {
    Env e;
    CHECK(!e.cancel.cancel("9999999999", "alice").ok);
    auto b = bookN(e, "alice", "SL", 1);
    auto other = e.cancel.cancel(b.ticket.pnr, "bob");
    CHECK(!other.ok);
    CHECK_CONTAINS(other.message, "own");
    CHECK(e.cancel.cancel(b.ticket.pnr, "alice").ok);
    auto again = e.cancel.cancel(b.ticket.pnr, "alice");
    CHECK(!again.ok);
    CHECK_CONTAINS(again.message, "already");

    auto b2 = bookN(e, "alice", "SL", 1);
    e.now = M(kDate, "19:20");
    auto late = e.cancel.cancel(b2.ticket.pnr, "alice");
    CHECK(!late.ok);
    CHECK_CONTAINS(late.message, "departed");
}
TEST(TC_Cancel_06_PromotionRacThenWaitlist) {  // RRS-F-032 (TC-Cancel-03 in the STP)
    Env e;
    auto A = bookN(e, "alice", "SL", 2), B = bookN(e, "bob", "SL", 2);  // 4 confirmed
    auto C = bookN(e, "alice", "SL", 1), D = bookN(e, "bob", "SL", 1);  // 2 RAC
    auto E = bookN(e, "alice", "SL", 1), F = bookN(e, "bob", "SL", 1);  // 2 WL
    CHECK(C.ticket.status == Status::RAC && E.ticket.status == Status::Waitlist);
    auto r = e.cancel.cancel(A.ticket.pnr, "alice");  // frees 2 berths
    CHECK(r.ok);
    CHECK(lookupPnr(e.db, C.ticket.pnr)->status == Status::Confirmed);
    CHECK(lookupPnr(e.db, D.ticket.pnr)->status == Status::Confirmed);
    CHECK(lookupPnr(e.db, E.ticket.pnr)->status == Status::RAC);
    CHECK(lookupPnr(e.db, F.ticket.pnr)->status == Status::RAC);
    CHECK_EQ(r.promotedPnrs.size(), size_t(4));
    CHECK(lookupPnr(e.db, B.ticket.pnr)->status == Status::Confirmed);
}
TEST(TC_Cancel_07_PromotionIsFirstComeFirstServed) {
    Env e;
    auto A = bookN(e, "alice", "SL", 1);
    for (int i = 0; i < 3; ++i) bookN(e, "bob", "SL", 1);  // confirmed full
    auto C = bookN(e, "alice", "SL", 1), D = bookN(e, "bob", "SL", 1);  // RAC order C, D
    e.cancel.cancel(A.ticket.pnr, "alice");  // one berth
    CHECK(lookupPnr(e.db, C.ticket.pnr)->status == Status::Confirmed);
    CHECK(lookupPnr(e.db, D.ticket.pnr)->status == Status::RAC);
}
TEST(TC_Cancel_08_WaitlistCannotJumpAheadOfRac) {
    Env e;
    auto s1 = bookN(e, "alice", "SL", 1);
    for (int i = 0; i < 3; ++i) bookN(e, "bob", "SL", 1);   // 4 confirmed
    auto R = bookN(e, "alice", "SL", 2);                    // 2-pax RAC (RAC full)
    auto W = bookN(e, "bob", "SL", 1);                      // WL
    CHECK(R.ticket.status == Status::RAC && W.ticket.status == Status::Waitlist);
    e.cancel.cancel(s1.ticket.pnr, "alice");                // 1 berth: RAC(2) cannot use it
    CHECK(lookupPnr(e.db, R.ticket.pnr)->status == Status::RAC);
    CHECK(lookupPnr(e.db, W.ticket.pnr)->status == Status::Waitlist);  // must not skip the RAC ticket
}
TEST(TC_Cancel_09_CancellingRacMovesWaitlistIntoRac) {
    Env e;
    for (int i = 0; i < 4; ++i) bookN(e, "bob", "SL", 1);
    auto R1 = bookN(e, "alice", "SL", 1); bookN(e, "alice", "SL", 1);
    auto W = bookN(e, "bob", "SL", 1);
    e.cancel.cancel(R1.ticket.pnr, "alice");
    CHECK(lookupPnr(e.db, W.ticket.pnr)->status == Status::RAC);
}
TEST(TC_Cancel_10_PromotionOnlyAffectsSameTrainClassDate) {
    Env e;
    auto a = bookN(e, "alice", "3A", 2);              // 3A confirmed full
    auto w = bookN(e, "bob", "3A", 1);                // 3A RAC
    auto other = e.booking.book(req("bob", "SL", 1, kGoodCard, "2026-10-11"));
    e.cancel.cancel(other.ticket.pnr, "bob");         // different class/date
    CHECK(lookupPnr(e.db, w.ticket.pnr)->status == Status::RAC);
    (void)a;
}
TEST(TC_Cancel_11_FreedSeatsBookableAgainAndPnrNotReused) {
    Env e;
    auto a = bookN(e, "alice", "SL", 4);
    e.cancel.cancel(a.ticket.pnr, "alice");
    auto b = bookN(e, "bob", "SL", 1);
    CHECK(b.ticket.status == Status::Confirmed);
    CHECK(b.ticket.pnr != a.ticket.pnr);
}
