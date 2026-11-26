#pragma once

#include "CsvStorage.h"

#include <string>
#include <functional>

namespace timetracker {

struct OperationResult {
    bool success = false;
    std::string message;
};

class TimerController {
public:
    explicit TimerController(CsvStorage& storage, std::function<Timestamp()> clock = now);

    OperationResult start(const std::string& taskId);
    OperationResult stop();
    bool getActive(TimeEntry& entry, Task& task, Project& project, std::string& error);

private:
    Timestamp nowUtc() const;
    CsvStorage& storage_;
    std::function<Timestamp()> clock_;
};

}
