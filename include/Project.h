#pragma once

#include <string>

namespace timetracker {

class Project {
public:
    Project() = default;
    Project(std::string id, std::string name);

    const std::string& id() const;
    const std::string& name() const;

private:
    std::string id_;
    std::string name_;
};

}
