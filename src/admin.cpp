
#include "rrs/admin.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <map>
#include <set>
#include <string>

#include "rrs/util.h"

namespace rrs {
namespace {

bool validTrain(const Train& train) {
    if (train.number <= 0 ||
        trim(train.name).empty() ||
        trim(train.source).empty() ||
        trim(train.destination).empty()) {
        return false;
    }

    if (iequals(trim(train.source), trim(train.destination))) {
        return false;
    }

    int hour = 0;
    int minute = 0;

    if (!parseTime(train.departure, hour, minute)) {
        return false;
    }

    if (train.classes.empty()) {
        return false;
    }

    std::set<std::string> classCodes;

    for (const auto& cls : train.classes) {
        const std::string code = trim(cls.code);

        if (code.empty() ||
            cls.totalSeats <= 0 ||
            cls.racSeats < 0 ||
            cls.wlLimit < 0 ||
            cls.fare <= 0 ||
            cls.racSeats > cls.totalSeats ||
            !classCodes.insert(code).second) {
            return false;
        }
    }

    return true;
}

}  // namespace

bool AdminService::logAction(const std::string& action,
                             const std::string& target) {
    std::ofstream log(logPath_, std::ios::app);
    if (!log) {
        return false;
    }

    const std::time_t now = clock_ ? clock_() : std::time(nullptr);
    std::tm timestamp{};
#if defined(_WIN32)
    localtime_s(&timestamp, &now);
#else
    localtime_r(&now, &timestamp);
#endif

    log << std::put_time(&timestamp, "%Y-%m-%d %H:%M:%S")
        << " | admin=" << adminUser_
        << " | action=" << action
        << " | target=" << target << '\n';
    return static_cast<bool>(log);
}

AdminResult AdminService::loggedSuccess(const std::string& successMessage,
                                        const std::string& action,
                                        const std::string& target) {
    if (!logAction(action, target)) {
        return {false, "Action completed, but the admin log could not be written."};
    }
    return {true, successMessage};
}

AdminResult AdminService::addTrain(const Train& train) {
    if (!adminAuthorized_) return {false, "Admin access required."};
    if (!validTrain(train)) {
        return {false, "Invalid train details. Please check all fields."};
    }

    const auto existing = std::find_if(
        db_.trains.begin(),
        db_.trains.end(),
        [&train](const Train& current) {
            return current.number == train.number;
        }
    );

    if (existing != db_.trains.end()) {
        return {false, "A train with this number already exists."};
    }

    Train newTrain = train;
    newTrain.name = trim(newTrain.name);
    newTrain.source = trim(newTrain.source);
    newTrain.destination = trim(newTrain.destination);
    newTrain.active = true;

    for (auto& cls : newTrain.classes) {
        cls.code = trim(cls.code);
    }

    db_.trains.push_back(newTrain);

    return loggedSuccess("Train added successfully.", "ADD_TRAIN", std::to_string(train.number));
}

AdminResult AdminService::modifyTrain(
    int trainNumber,
    const Train& updatedTrain
) {
    if (!adminAuthorized_) return {false, "Admin access required."};
    auto existing = std::find_if(
        db_.trains.begin(),
        db_.trains.end(),
        [trainNumber](const Train& train) {
            return train.number == trainNumber;
        }
    );

    if (existing == db_.trains.end()) {
        return {false, "Train not found."};
    }

    if (updatedTrain.number != trainNumber) {
        return {false, "Modifying the train number is not allowed."};
    }

    if (!validTrain(updatedTrain)) {
        return {false, "Invalid updated train details."};
    }

    Train replacement = updatedTrain;
    replacement.name = trim(replacement.name);
    replacement.source = trim(replacement.source);
    replacement.destination = trim(replacement.destination);
    replacement.active = existing->active;

    for (auto& cls : replacement.classes) {
        cls.code = trim(cls.code);
    }

    *existing = replacement;

    return loggedSuccess("Train updated successfully.", "MODIFY_TRAIN", std::to_string(trainNumber));
}

AdminResult AdminService::deactivateTrain(int trainNumber) {
    if (!adminAuthorized_) return {false, "Admin access required."};
    auto existing = std::find_if(
        db_.trains.begin(),
        db_.trains.end(),
        [trainNumber](const Train& train) {
            return train.number == trainNumber;
        }
    );

    if (existing == db_.trains.end()) {
        return {false, "Train not found."};
    }

    if (!existing->active) {
        return {false, "Train is already inactive."};
    }

    existing->active = false;

    return loggedSuccess("Train deactivated successfully.", "DEACTIVATE_TRAIN", std::to_string(trainNumber));
}

AdminResult AdminService::updateClassFareAndQuota(
    int trainNumber,
    const std::string& classCode,
    int fare,
    int totalSeats,
    int racSeats,
    int wlLimit
) {
    if (!adminAuthorized_) return {false, "Admin access required."};
    auto trainIt = std::find_if(
        db_.trains.begin(),
        db_.trains.end(),
        [trainNumber](const Train& train) {
            return train.number == trainNumber;
        }
    );

    if (trainIt == db_.trains.end()) {
        return {false, "Train not found."};
    }

    const std::string requestedCode = trim(classCode);

    auto classIt = std::find_if(
        trainIt->classes.begin(),
        trainIt->classes.end(),
        [&requestedCode](const ClassInfo& cls) {
            return iequals(trim(cls.code), requestedCode);
        }
    );

    if (classIt == trainIt->classes.end()) {
        return {false, "Class not found."};
    }

    if (fare <= 0 ||
        totalSeats <= 0 ||
        racSeats < 0 ||
        wlLimit < 0 ||
        racSeats > totalSeats) {
        return {false, "Invalid fare or quota values."};
    }

    // Use the same time convention as the rest of the application.
    const long long now = clock_ ? clock_() : currentMinutes();

    // Index 0 = confirmed passengers, 1 = RAC passengers,
    // 2 = waitlisted passengers.
    std::map<std::string, std::array<int, 3>> bookedByDate;

    for (const Ticket& ticket : db_.tickets) {
        if (ticket.trainNumber != trainNumber ||
            !iequals(trim(ticket.classCode), requestedCode) ||
            ticket.status == Status::Cancelled) {
            continue;
        }

        long long departure = 0;

        if (!toEpochMinutes(
                ticket.journeyDate,
                trainIt->departure,
                departure) ||
            departure <= now) {
            continue;
        }

        const int passengerCount =
            static_cast<int>(ticket.passengers.size());

        auto& counts = bookedByDate[ticket.journeyDate];

        if (ticket.status == Status::Confirmed) {
            counts[0] += passengerCount;
        } else if (ticket.status == Status::RAC) {
            counts[1] += passengerCount;
        } else if (ticket.status == Status::Waitlist) {
            counts[2] += passengerCount;
        }
    }

    // Check every future journey independently.
    for (const auto& entry : bookedByDate) {
        const auto& counts = entry.second;

        if (counts[0] > totalSeats) {
            return {
                false,
                "Total seats cannot be lower than confirmed bookings."
            };
        }

        if (counts[1] > racSeats) {
            return {
                false,
                "RAC seats cannot be lower than existing RAC bookings."
            };
        }

        if (counts[2] > wlLimit) {
            return {
                false,
                "Waitlist limit cannot be lower than existing waitlist bookings."
            };
        }
    }

    // Apply changes only after all validation succeeds.
    // Existing tickets retain their original totalFare.
    classIt->fare = fare;
    classIt->totalSeats = totalSeats;
    classIt->racSeats = racSeats;
    classIt->wlLimit = wlLimit;

    return loggedSuccess("Fare and quotas updated successfully.", "UPDATE_FARE_QUOTA", std::to_string(trainNumber) + ":" + requestedCode);
}

}  // namespace rrs
