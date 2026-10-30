
#include "test_helpers.h"
#include "rrs/admin.h"

TEST(TC_Admin_01_AddTrainSuccessfully) {
    Env e;
    AdminService admin(e.db);

    Train train;
    train.number = 22691;
    train.name = "Rajdhani Express";
    train.source = "Bengaluru";
    train.destination = "New Delhi";
    train.departure = "20:00";
    train.classes = {{"3A", 60, 6, 12, 2760}};

    const auto result = admin.addTrain(train);

    CHECK(result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(2));
    CHECK_EQ(e.db.trains[1].number, 22691);
    CHECK(e.db.trains[1].active);
}

TEST(TC_Admin_02_RejectDuplicateTrainNumber) {
    Env e;
    AdminService admin(e.db);

    const auto result = admin.addTrain(e.db.trains[0]);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(1));
}

TEST(TC_Admin_03_RejectInvalidTrain) {
    Env e;
    AdminService admin(e.db);

    Train train;
    train.number = 30001;
    train.name = "Invalid Train";
    train.source = "Bengaluru";
    train.destination = "New Delhi";
    train.departure = "25:90";
    train.classes = {{"SL", 10, 2, 2, 500}};

    const auto result = admin.addTrain(train);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(1));
}

TEST(TC_Admin_04_ModifyTrainDetails) {
    Env e;
    AdminService admin(e.db);

    Train updated = e.db.trains[0];
    updated.name = "Karnataka Express Updated";
    updated.departure = "20:30";
    updated.classes[0].fare = 600;

    const auto result = admin.modifyTrain(12627, updated);

    CHECK(result.ok);
    CHECK_EQ(e.db.trains[0].name, std::string("Karnataka Express Updated"));
    CHECK_EQ(e.db.trains[0].departure, std::string("20:30"));
    CHECK_EQ(e.db.trains[0].classes[0].fare, 600);
}

TEST(TC_Admin_05_ModifyUnknownTrain) {
    Env e;
    AdminService admin(e.db);

    const auto result = admin.modifyTrain(99999, e.db.trains[0]);

    CHECK(!result.ok);
}

TEST(TC_Admin_06_DeactivateTrain) {
    Env e;
    AdminService admin(e.db);

    const auto result = admin.deactivateTrain(12627);

    CHECK(result.ok);
    CHECK(!e.db.trains[0].active);
    CHECK(e.search.search({"Bengaluru", "New Delhi", kDate, ""}).rows.empty());
}

TEST(TC_Admin_07_DeactivateUnknownTrain) {
    Env e;
    AdminService admin(e.db);

    const auto result = admin.deactivateTrain(99999);

    CHECK(!result.ok);
    CHECK(e.db.trains[0].active);
}

TEST(TC_Admin_08_RejectDuplicateClassCodes) {
    Env e;
    AdminService admin(e.db);

    Train train;
    train.number = 30002;
    train.name = "Test Express";
    train.source = "Chennai";
    train.destination = "Bengaluru";
    train.departure = "10:30";
    train.classes = {
        {"SL", 10, 2, 2, 500},
        {"SL", 20, 3, 4, 600}
    };

    const auto result = admin.addTrain(train);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(1));
}

TEST(TC_Admin_09_UpdateClassFareAndQuota) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto result =
        admin.updateClassFareAndQuota(12627, "SL", 700, 4, 2, 2);

    CHECK(result.ok);
    CHECK_EQ(e.db.trains[0].classes[0].fare, 700);
    CHECK_EQ(e.db.trains[0].classes[0].totalSeats, 4);
    CHECK_EQ(e.db.trains[0].classes[0].racSeats, 2);
    CHECK_EQ(e.db.trains[0].classes[0].wlLimit, 2);
}

TEST(TC_Admin_10_RejectQuotaBelowConfirmedBookings) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto booking = bookN(e, "alice", "SL", 2);
    CHECK(booking.ok);

    const auto result =
        admin.updateClassFareAndQuota(12627, "SL", 600, 1, 2, 2);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains[0].classes[0].totalSeats, 4);
}

TEST(TC_Admin_11_OldTicketFareRemainsUnchanged) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto booking = bookN(e, "alice", "SL", 1);
    CHECK(booking.ok);

    const int originalFare = e.db.tickets.back().totalFare;

    const auto result =
        admin.updateClassFareAndQuota(12627, "SL", 700, 4, 2, 2);

    CHECK(result.ok);
    CHECK_EQ(e.db.tickets.back().totalFare, originalFare);
}

TEST(TC_Admin_12_RefundUsesOriginalTicketFare) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto booking = bookN(e, "alice", "SL", 1);
    CHECK(booking.ok);

    const Ticket oldTicket = e.db.tickets.back();
    const int originalFare = oldTicket.totalFare;

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 700, 4, 2, 2);

    CHECK(update.ok);

    long long departure = 0;
    CHECK(toEpochMinutes(oldTicket.journeyDate, "19:20", departure));

    const int expectedRefund =
        calculateRefund(oldTicket, departure, e.now);

    const auto cancellation =
        e.cancel.cancel(oldTicket.pnr, "alice");

    CHECK(cancellation.ok);
    CHECK_EQ(cancellation.refund, expectedRefund);
    CHECK_EQ(e.db.tickets.back().totalFare, originalFare);
}

TEST(TC_Admin_13_RejectQuotaBelowAggregateBookings) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto first = bookN(e, "alice", "SL", 2);
    CHECK(first.ok);

    const auto second = bookN(e, "bob", "SL", 2);
    CHECK(second.ok);

    const auto result =
        admin.updateClassFareAndQuota(12627, "SL", 600, 3, 2, 2);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains[0].classes[0].totalSeats, 4);
}

TEST(TC_Admin_14_FareDecreaseAffectsNewBookingsOnly) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto oldBooking = bookN(e, "alice", "SL", 1);
    CHECK(oldBooking.ok);

    const int originalFare = oldBooking.ticket.totalFare;

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 400, 4, 2, 2);

    CHECK(update.ok);

    const auto newBooking = e.booking.book(req("bob", "SL", 1));
    CHECK(newBooking.ok);

    CHECK_EQ(newBooking.ticket.totalFare, 400);
    CHECK_EQ(oldBooking.ticket.totalFare, originalFare);
}

TEST(TC_Admin_15_RejectRacQuotaBelowExistingBookings) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto confirmed = bookN(e, "alice", "SL", 4);
    CHECK(confirmed.ok);

    const auto racBooking = bookN(e, "bob", "SL", 1);
    CHECK(racBooking.ok);
    CHECK(racBooking.ticket.status == Status::RAC);

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 500, 4, 0, 2);

    CHECK(!update.ok);
    CHECK_EQ(e.db.trains[0].classes[0].racSeats, 2);
}

TEST(TC_Admin_16_RejectWaitlistLimitBelowExistingBookings) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto confirmed = bookN(e, "alice", "SL", 4);
    CHECK(confirmed.ok);

    const auto racBooking = bookN(e, "bob", "SL", 2);
    CHECK(racBooking.ok);
    CHECK(racBooking.ticket.status == Status::RAC);

    const auto waitlistBooking =
        e.booking.book(req("alice", "SL", 1));

    CHECK(waitlistBooking.ok);
    CHECK(waitlistBooking.ticket.status == Status::Waitlist);

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 500, 4, 2, 0);

    CHECK(!update.ok);
    CHECK_EQ(e.db.trains[0].classes[0].wlLimit, 2);
}

TEST(TC_Admin_17_InvalidFareDoesNotChangeConfiguration) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 0, 4, 2, 2);

    CHECK(!update.ok);
    CHECK_EQ(e.db.trains[0].classes[0].fare, 500);
    CHECK_EQ(e.db.trains[0].classes[0].totalSeats, 4);
}

TEST(TC_Admin_18_QuotaValidationIsPerJourneyDate) {
    Env e;
    AdminService admin(e.db, e.clock);

    const auto first = e.booking.book(
        req("alice", "SL", 4, kGoodCard, "2026-10-10")
    );
    CHECK(first.ok);

    const auto second = e.booking.book(
        req("bob", "SL", 4, kGoodCard, "2026-10-11")
    );
    CHECK(second.ok);

    const auto update =
        admin.updateClassFareAndQuota(12627, "SL", 600, 4, 2, 2);

    CHECK(update.ok);
    CHECK_EQ(e.db.trains[0].classes[0].totalSeats, 4);
}


TEST(TC_AdminAccess_RejectsPassengerActions) {
    Env e;
    AdminService admin(e.db, e.clock);
    admin.setAdminAuthorized(false);

    const auto add = admin.addTrain(e.db.trains[0]);
    CHECK(!add.ok);
    CHECK_CONTAINS(add.message, "Admin access required.");

    const auto modify = admin.modifyTrain(12627, e.db.trains[0]);
    CHECK(!modify.ok);
    CHECK_CONTAINS(modify.message, "Admin access required.");

    const auto deactivate = admin.deactivateTrain(12627);
    CHECK(!deactivate.ok);
    CHECK_CONTAINS(deactivate.message, "Admin access required.");

    const auto fare = admin.updateClassFareAndQuota(12627, "SL", 600, 4, 2, 2);
    CHECK(!fare.ok);
    CHECK_CONTAINS(fare.message, "Admin access required.");

    CHECK(e.db.trains[0].active);
    CHECK_EQ(e.db.trains[0].classes[0].fare, 500);
}
