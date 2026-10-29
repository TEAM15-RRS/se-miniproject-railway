
#include "test_helpers.h"
#include "rrs/admin.h"

TEST(TC_Admin_01_AddTrainSuccessfully) {
    Env e;
    AdminService admin(e.db);

    Train t;
    t.number = 22691;
    t.name = "Rajdhani Express";
    t.source = "Bengaluru";
    t.destination = "New Delhi";
    t.departure = "20:00";
    t.classes = {{"3A", 60, 6, 12, 2760}};

    auto result = admin.addTrain(t);

    CHECK(result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(2));
    CHECK_EQ(e.db.trains[1].number, 22691);
    CHECK(e.db.trains[1].active);
}

TEST(TC_Admin_02_RejectDuplicateTrainNumber) {
    Env e;
    AdminService admin(e.db);

    Train t = e.db.trains[0];

    auto result = admin.addTrain(t);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(1));
}

TEST(TC_Admin_03_RejectInvalidTrain) {
    Env e;
    AdminService admin(e.db);

    Train t;
    t.number = 30001;
    t.name = "Invalid Train";
    t.source = "Bengaluru";
    t.destination = "New Delhi";
    t.departure = "25:90";
    t.classes = {{"SL", 10, 2, 2, 500}};

    auto result = admin.addTrain(t);

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

    auto result = admin.modifyTrain(12627, updated);

    CHECK(result.ok);
    CHECK_EQ(e.db.trains[0].name, std::string("Karnataka Express Updated"));
    CHECK_EQ(e.db.trains[0].departure, std::string("20:30"));
    CHECK_EQ(e.db.trains[0].classes[0].fare, 600);
}

TEST(TC_Admin_05_ModifyUnknownTrain) {
    Env e;
    AdminService admin(e.db);

    Train updated = e.db.trains[0];

    auto result = admin.modifyTrain(99999, updated);

    CHECK(!result.ok);
}

TEST(TC_Admin_06_DeactivateTrain) {
    Env e;
    AdminService admin(e.db);

    auto result = admin.deactivateTrain(12627);

    CHECK(result.ok);
    CHECK(!e.db.trains[0].active);
    CHECK(e.search.search({"Bengaluru", "New Delhi", kDate, ""}).rows.empty());
}

TEST(TC_Admin_07_DeactivateUnknownTrain) {
    Env e;
    AdminService admin(e.db);

    auto result = admin.deactivateTrain(99999);

    CHECK(!result.ok);
    CHECK(e.db.trains[0].active);
}

TEST(TC_Admin_08_RejectDuplicateClassCodes) {
    Env e;
    AdminService admin(e.db);

    Train t;
    t.number = 30002;
    t.name = "Test Express";
    t.source = "Chennai";
    t.destination = "Bengaluru";
    t.departure = "10:30";
    t.classes = {
        {"SL", 10, 2, 2, 500},
        {"SL", 20, 3, 4, 600}
    };

    auto result = admin.addTrain(t);

    CHECK(!result.ok);
    CHECK_EQ(e.db.trains.size(), size_t(1));
}
