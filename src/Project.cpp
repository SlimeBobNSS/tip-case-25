#include "Project.h"

#include <utility>

namespace timetracker {

Project::Project(std::string id, std::string name)
    : id_(std::move(id)), name_(std::move(name)) {}

const std::string& Project::id() const { return id_; }
const std::string& Project::name() const { return name_; }

}
