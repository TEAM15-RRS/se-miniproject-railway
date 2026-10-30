#include "test_helpers.h"
#include "rrs/admin.h"

TEST(TC_AdminReports_01_GroupCountsAndRevenue) {
    Env e;
    AdminService admin(e.db);

    Ticket t1;
    t1.pnr = "PNR001";
    t1.trainNumber = 12627;
    t1.journeyDate = "2026-10-10";
    t1.status = Status::Confirmed;
    t1.totalFare = 1000;

    Ticket t2;
    t2.pnr = "PNR002";
    t2.trainNumber = 12627;
    t2.journeyDate = "2026-10-10";
    t2.status = Status::RAC;
    t2.totalFare = 500;

    Ticket t3;
    t3.pnr = "PNR003";
    t3.trainNumber = 12627;
    t3.journeyDate = "2026-10-10";
    t3.status = Status::Waitlist;
    t3.totalFare = 300;

    Ticket t4;
    t4.pnr = "PNR004";
    t4.trainNumber = 12627;
    t4.journeyDate = "2026-10-10";
    t4.status = Status::Cancelled;
    t4.totalFare = 800;
    t4.refundAmount = 600;

    e.db.tickets = {t1, t2, t3, t4};

    const auto reports = admin.generateReports();

    CHECK_EQ(reports.size(), size_t(1));
    CHECK_EQ(reports[0].trainNumber, 12627);
    CHECK_EQ(reports[0].journeyDate, std::string("2026-10-10"));

    CHECK_EQ(reports[0].confirmedCount, 1);
    CHECK_EQ(reports[0].racCount, 1);
    CHECK_EQ(reports[0].waitlistCount, 1);
    CHECK_EQ(reports[0].cancelledCount, 1);

    // Revenue = 1000 + 500 + 300 = 1800.
    // Cancelled ticket's fare and refund are excluded.
    CHECK_EQ(reports[0].revenue, 1800);
}

TEST(TC_AdminReports_02_EmptyDatabase) {
    Env e;
    AdminService admin(e.db);

    const auto reports = admin.generateReports();

    CHECK(reports.empty());
}

TEST(TC_AdminReports_03_GroupByTrainAndDate) {
    Env e;
    AdminService admin(e.db);

    Ticket t1;
    t1.trainNumber = 12627;
    t1.journeyDate = "2026-10-10";
    t1.status = Status::Confirmed;
    t1.totalFare = 500;

    Ticket t2;
    t2.trainNumber = 12627;
    t2.journeyDate = "2026-10-11";
    t2.status = Status::Confirmed;
    t2.totalFare = 600;

    Ticket t3;
    t3.trainNumber = 12628;
    t3.journeyDate = "2026-10-10";
    t3.status = Status::RAC;
    t3.totalFare = 400;

    e.db.tickets = {t1, t2, t3};

    const auto reports = admin.generateReports();

    CHECK_EQ(reports.size(), size_t(3));

    CHECK_EQ(reports[0].trainNumber, 12627);
    CHECK_EQ(reports[0].journeyDate, std::string("2026-10-10"));
    CHECK_EQ(reports[0].revenue, 500);

    CHECK_EQ(reports[1].trainNumber, 12627);
    CHECK_EQ(reports[1].journeyDate, std::string("2026-10-11"));
    CHECK_EQ(reports[1].revenue, 600);

    CHECK_EQ(reports[2].trainNumber, 12628);
    CHECK_EQ(reports[2].journeyDate, std::string("2026-10-10"));
    CHECK_EQ(reports[2].revenue, 400);
}