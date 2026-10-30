
#include "rrs/admin.h"

#include <algorithm>
#include <array>
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

AdminResult AdminService::addTrain(const Train& train) {
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

    return {true, "Train added successfully."};
}

AdminResult AdminService::modifyTrain(
    int trainNumber,
    const Train& updatedTrain
) {
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

    return {true, "Train updated successfully."};
}

AdminResult AdminService::deactivateTrain(int trainNumber) {
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

    return {true, "Train deactivated successfully."};
}

AdminResult AdminService::updateClassFareAndQuota(
    int trainNumber,
    const std::string& classCode,
    int fare,
    int totalSeats,
    int racSeats,
    int wlLimit
) {
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

    return {true, "Fare and quotas updated successfully."};
}

}  // namespace rrs
