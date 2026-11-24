#pragma once

#include <string>

namespace timetracker {

class Task {
public:
    Task() = default;
    Task(std::string id, std::string projectId, std::string name);

    const std::string& id() const;
    const std::string& projectId() const;
    const std::string& name() const;

private:
    std::string id_;
    std::string projectId_;
    std::string name_;
};

}
