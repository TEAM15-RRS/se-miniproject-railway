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
    explicit AdminService(
        Database& db,
        Clock clock = {},
        std::string adminUser = "admin",
        std::string logPath = "admin_log.txt"
    )
        : db_(db),
          clock_(std::move(clock)),
          adminUser_(std::move(adminUser)),
          logPath_(std::move(logPath)) {}

    void setAdminUser(const std::string& username) {
        adminUser_ = username;
    }

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
   bool logAction(const std::string& action, const std::string& target);
    AdminResult loggedSuccess(
        const std::string& successMessage,
        const std::string& action,
        const std::string& target
    );

    Database& db_;
    Clock clock_;
    bool adminAuthorized_ = true;
    std::string adminUser_;
    std::string logPath_;
};

}  // namespace rrs
