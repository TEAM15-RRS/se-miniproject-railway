#pragma once

#include <string>
#include <utility>
#include <vector>

#include "rrs/types.h"

namespace rrs {

struct AdminResult {
    bool ok = false;
    std::string message;
};

struct TrainDateReport {
    int trainNumber = 0;
    std::string journeyDate;

    int confirmedCount = 0;
    int racCount = 0;
    int waitlistCount = 0;
    int cancelledCount = 0;

    int revenue = 0;
};

class AdminService {
public:
    explicit AdminService(Database& db, Clock clock = {})
        : db_(db), clock_(std::move(clock)) {}

    AdminResult addTrain(const Train& train);
    AdminResult modifyTrain(int trainNumber, const Train& updatedTrain);
    AdminResult deactivateTrain(int trainNumber);

    AdminResult updateClassFareAndQuota(
        int trainNumber,
        const std::string& classCode,
        int fare,
        int totalSeats,
        int racSeats,
        int wlLimit
    );
    std::vector<TrainDateReport> generateReports() const;

private:
    Database& db_;
    Clock clock_;
};

}  // namespace rrs
