
#pragma once

#include <string>
#include <utility>

#include "rrs/types.h"

namespace rrs {

struct AdminResult {
    bool ok = false;
    std::string message;
};

class AdminService {
public:
    explicit AdminService(Database& db, Clock clock = {})
        : db_(db), clock_(std::move(clock)) {}

    void setAdminAuthorized(bool authorized) { adminAuthorized_ = authorized; }

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

private:
    Database& db_;
    Clock clock_;
    bool adminAuthorized_ = true;
};

}  // namespace rrs
