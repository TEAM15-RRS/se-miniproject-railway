
#include "rrs/admin.h"

#include <algorithm>
#include <cctype>
#include <set>

#include "rrs/util.h"

namespace rrs {
namespace {

bool validTrain(const Train& train) {
    if (train.number <= 0 || trim(train.name).empty() ||
        trim(train.source).empty() || trim(train.destination).empty()) {
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

    for (const auto& c : train.classes) {
        std::string code = trim(c.code);

        if (code.empty() || c.totalSeats <= 0 || c.racSeats < 0 ||
            c.wlLimit < 0 || c.fare <= 0) {
            return false;
        }

        if (c.racSeats > c.totalSeats ||
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

    auto existing = std::find_if(
        db_.trains.begin(), db_.trains.end(),
        [&](const Train& t) {
            return t.number == train.number;
        });

    if (existing != db_.trains.end()) {
        return {false, "A train with this number already exists."};
    }

    Train newTrain = train;
    newTrain.name = trim(newTrain.name);
    newTrain.source = trim(newTrain.source);
    newTrain.destination = trim(newTrain.destination);
    newTrain.active = true;

    for (auto& c : newTrain.classes) {
        c.code = trim(c.code);
    }

    db_.trains.push_back(newTrain);

    return {true, "Train added successfully."};
}

AdminResult AdminService::modifyTrain(
    int trainNumber, const Train& updatedTrain) {

    auto existing = std::find_if(
        db_.trains.begin(), db_.trains.end(),
        [&](const Train& t) {
            return t.number == trainNumber;
        });

    if (existing == db_.trains.end()) {
        return {false, "Train not found."};
    }

    if (updatedTrain.number != trainNumber) {
        return {false, "Modifying the train number is not allowed."};
    }

    if (!validTrain(updatedTrain)) {
        return {false, "Invalid updated train details."};
    }

    for (const auto& t : db_.trains) {
        if (t.number == updatedTrain.number &&
            t.number != trainNumber) {
            return {false, "Another train already uses this number."};
        }
    }

    Train replacement = updatedTrain;
    replacement.name = trim(replacement.name);
    replacement.source = trim(replacement.source);
    replacement.destination = trim(replacement.destination);
    replacement.active = existing->active;

    for (auto& c : replacement.classes) {
        c.code = trim(c.code);
    }

    *existing = replacement;

    return {true, "Train updated successfully."};
}

AdminResult AdminService::deactivateTrain(int trainNumber) {
    auto existing = std::find_if(
        db_.trains.begin(), db_.trains.end(),
        [&](const Train& t) {
            return t.number == trainNumber;
        });

    if (existing == db_.trains.end()) {
        return {false, "Train not found."};
    }

    if (!existing->active) {
        return {false, "Train is already inactive."};
    }

    existing->active = false;

    return {true, "Train deactivated successfully."};
}

}  // namespace rrs
