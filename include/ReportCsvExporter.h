#pragma once

#include "Report.h"

#include <filesystem>
#include <string>

namespace timetracker {

class ReportCsvExporter {
public:
    explicit ReportCsvExporter(std::filesystem::path dataDirectory);
    bool write(const Report& report, const std::filesystem::path& path, std::string& error) const;

private:
    std::filesystem::path dataDirectory_;
};

}
