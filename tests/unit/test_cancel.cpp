#include "test_helpers.h"
#include "rrs/admin.h"

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
        {5 * 24 * 60,   750},  // 5 days before -> 75%
        {24 * 60 + 1,   750},  // 24h01 -> just over 24h -> 75%
        {24 * 60,       500},  // exactly 24h -> 50%
        {12 * 60 + 1,   500},  // 12h01 -> 50%
        {12 * 60,       500},  // exactly 12h -> 50%
        {12 * 60 - 1,   250},  // 11h59 -> 25%
        {1,             250},  // 1 minute before departure -> 25%
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
TEST(TC_Cancel_03_RefundRoundsDownToWholeRupees) {
    Env e;
    e.db.trains[0].classes[0].fare = 15;   // 1 passenger: 75% of 15 = 11.25 -> 11
    auto b = bookN(e, "alice", "SL", 1);
    CHECK_EQ(e.cancel.cancel(b.ticket.pnr, "alice").refund, 11);
}
TEST(TC_Cancel_04_WaitlistTicketUsesSameSlabs) {  // SRS 4.4: confirmed or waitlisted
    Env e;
    for (int i = 0; i < 6; ++i) bookN(e, "alice", "SL", 1);
    auto w = bookN(e, "bob", "SL", 1);
    CHECK(w.ticket.status == Status::Waitlist);
    setBefore(e, 60);  // 1 hour before departure -> 25% of 500
    CHECK_EQ(e.cancel.cancel(w.ticket.pnr, "bob").refund, 125);
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


// ---- Regression testing after administrative changes ----

// Issue #24: Original ticket fare must be retained after a fare change.
TEST(TC_Cancel_15_RefundUsesOriginalFareAfterFareChange) {
    Env e;
    AdminService admin(e.db, e.clock);

    auto booking = bookN(e, "alice", "SL", 2);
    CHECK(booking.ok);
    if (!booking.ok) return;

    const int originalFare = booking.ticket.totalFare;
    CHECK_EQ(originalFare, 1000);

    auto update = admin.updateClassFareAndQuota(
        12627, "SL", 900, 4, 2, 2
    );

    CHECK(update.ok);
    if (!update.ok) return;

    CHECK_EQ(e.db.trains[0].classes[0].fare, 900);
    CHECK_EQ(lookupPnr(e.db, booking.ticket.pnr)->totalFare, originalFare);

    // Cancel exactly 48 hours before departure.
    e.now = M(kDate, "19:20") - 48 * 60;

    auto cancellation = e.cancel.cancel(booking.ticket.pnr, "alice");

    CHECK(cancellation.ok);
    CHECK_EQ(cancellation.refund, 880);
    CHECK_EQ(
        lookupPnr(e.db, booking.ticket.pnr)->refundAmount,
        880
    );
    CHECK_EQ(
        lookupPnr(e.db, booking.ticket.pnr)->totalFare,
        originalFare
    );
}


// Issue #24: Existing tickets must remain cancellable after train deactivation.
TEST(TC_Cancel_16_RefundAfterTrainDeactivation) {
    Env e;
    AdminService admin(e.db, e.clock);

    auto booking = bookN(e, "alice", "SL", 2);
    CHECK(booking.ok);
    if (!booking.ok) return;

    const int originalFare = booking.ticket.totalFare;
    CHECK_EQ(originalFare, 1000);

    auto deactivation = admin.deactivateTrain(12627);

    CHECK(deactivation.ok);
    CHECK(!e.db.trains[0].active);

    e.now = M(kDate, "19:20") - 48 * 60;

    auto cancellation = e.cancel.cancel(booking.ticket.pnr, "alice");

    CHECK(cancellation.ok);
    CHECK_EQ(cancellation.refund, 880);

    const Ticket* cancelledTicket =
        lookupPnr(e.db, booking.ticket.pnr);

    CHECK(cancelledTicket != nullptr);

    if (cancelledTicket != nullptr) {
        CHECK(cancelledTicket->status == Status::Cancelled);
        CHECK_EQ(cancelledTicket->totalFare, originalFare);
        CHECK_EQ(cancelledTicket->refundAmount, 880);
    }
}


// Issue #24: Waitlist promotion must use updated RAC quota.
TEST(TC_Cancel_17_WaitlistPromotionAfterQuotaChange) {
    Env e;
    AdminService admin(e.db, e.clock);

    // Fill all four confirmed berths.
    auto C1 = bookN(e, "alice", "SL", 1);
    auto C2 = bookN(e, "bob", "SL", 1);
    auto C3 = bookN(e, "alice", "SL", 1);
    auto C4 = bookN(e, "bob", "SL", 1);

    // Fill both RAC slots.
    auto R1 = bookN(e, "alice", "SL", 1);
    auto R2 = bookN(e, "bob", "SL", 1);

    // Fill both waitlist slots.
    auto W1 = bookN(e, "alice", "SL", 1);
    auto W2 = bookN(e, "bob", "SL", 1);

    CHECK(C1.ok && C2.ok && C3.ok && C4.ok);
    CHECK(R1.ok && R2.ok && W1.ok && W2.ok);

    if (!(C1.ok && C2.ok && C3.ok && C4.ok &&
          R1.ok && R2.ok && W1.ok && W2.ok)) {
        return;
    }

    CHECK(C1.ticket.status == Status::Confirmed);
    CHECK(R1.ticket.status == Status::RAC);
    CHECK(W1.ticket.status == Status::Waitlist);

    // Increase RAC capacity from 2 to 3.
    auto quotaUpdate = admin.updateClassFareAndQuota(
        12627, "SL", 500, 4, 3, 2
    );

    CHECK(quotaUpdate.ok);
    if (!quotaUpdate.ok) return;

    CHECK_EQ(e.db.trains[0].classes[0].racSeats, 3);

    // Cancelling one confirmed ticket triggers promotion.
    auto cancellation = e.cancel.cancel(C1.ticket.pnr, "alice");

    CHECK(cancellation.ok);

    // The first RAC ticket should become confirmed.
    CHECK(
        lookupPnr(e.db, R1.ticket.pnr)->status == Status::Confirmed
    );

    // The second RAC ticket remains RAC.
    CHECK(
        lookupPnr(e.db, R2.ticket.pnr)->status == Status::RAC
    );

    // Both waitlisted tickets should now move into RAC
    // because the RAC quota was increased to three.
    CHECK(
        lookupPnr(e.db, W1.ticket.pnr)->status == Status::RAC
    );

    CHECK(
        lookupPnr(e.db, W2.ticket.pnr)->status == Status::RAC
    );

    CHECK_EQ(cancellation.promotedPnrs.size(), size_t(3));
}
