
#pragma once

#include <string>

#include "rrs/types.h"

namespace rrs {

struct AdminResult {
    bool ok = false;
    std::string message;
};

class AdminService {
public:
    explicit AdminService(Database& db) : db_(db) {}

    AdminResult addTrain(const Train& train);
    AdminResult modifyTrain(int trainNumber, const Train& updatedTrain);
    AdminResult deactivateTrain(int trainNumber);

private:
    Database& db_;
};

}  // namespace rrs
