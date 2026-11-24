#pragma once

#include <string>

namespace timetracker {

std::string generateUuid();
bool isUuid(const std::string& value);

}
