#pragma once
#include "DateTime.h"
namespace timetracker {
struct Period {
    Timestamp startUtc;
    Timestamp endUtc;
};
}
