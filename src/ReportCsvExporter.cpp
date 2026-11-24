#include "ReportCsvExporter.h"

#include <fstream>
#include <algorithm>
#include <cctype>
#include <system_error>

namespace timetracker {
namespace {

bool samePath(const std::filesystem::path& left, const std::filesystem::path& right) {
    auto a = left.generic_string();
    auto b = right.generic_string();
#ifdef _WIN32
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
#endif
    return a == b;
}

std::string field(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string result = "\"";
    for (char ch : value) result += ch == '"' ? "\"\"" : std::string(1, ch);
    return result + '"';
}

bool writeRow(std::ostream& output, const ReportRow* row) {
    if (!row) {
        output << "row_type,project_id,project_name,task_id,task_name,total_seconds\r\n";
    } else {
        output << field(row->rowType) << ',' << field(row->projectId) << ',' << field(row->projectName)
               << ',' << field(row->taskId) << ',' << field(row->taskName) << ','
               << row->durationSeconds << "\r\n";
    }
    return static_cast<bool>(output);
}

}

ReportCsvExporter::ReportCsvExporter(std::filesystem::path dataDirectory)
    : dataDirectory_(std::move(dataDirectory)) {}

bool ReportCsvExporter::write(const ReportResult& report, const std::filesystem::path& path,
                              std::string& error) const {
    std::error_code ec;
    const auto dataDirectory = std::filesystem::weakly_canonical(dataDirectory_, ec);
    if (ec) { error = "Не удалось определить каталог исходных данных: " + ec.message(); return false; }
    const auto absolute = std::filesystem::absolute(path, ec).lexically_normal();
    if (ec) { error = "Не удалось определить полный путь экспорта: " + ec.message(); return false; }
    const auto destination = std::filesystem::weakly_canonical(absolute, ec);
    if (ec) { error = "Не удалось нормализовать путь экспорта: " + ec.message(); return false; }
    for (const char* name : {"projects.csv", "tasks.csv", "sessions.csv"}) {
        if (samePath(destination, (dataDirectory / name).lexically_normal())) {
            error = "Нельзя экспортировать отчёт поверх файла исходных данных.";
            return false;
        }
    }
    if (std::filesystem::exists(destination, ec)) {
        error = "Файл назначения уже существует и не будет перезаписан.";
        return false;
    }
    if (ec) { error = "Не удалось проверить путь экспорта: " + ec.message(); return false; }
    if (!destination.parent_path().empty()) {
        std::filesystem::create_directories(destination.parent_path(), ec);
        if (ec) { error = "Не удалось создать каталог экспорта: " + ec.message(); return false; }
    }
    const auto temporary = destination.string() + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) { error = "Не удалось создать временный файл экспорта."; return false; }
    if (!writeRow(output, nullptr)) { error = "Ошибка записи заголовка отчёта."; return false; }
    for (const auto& row : report.rows) {
        if (!writeRow(output, &row)) { error = "Ошибка записи строк отчёта."; return false; }
    }
    output.flush();
    if (!output) { error = "Ошибка записи CSV-отчёта."; return false; }
    output.close();
    if (!output) { error = "Не удалось закрыть CSV-отчёт."; return false; }
    if (std::filesystem::exists(destination, ec)) {
        std::filesystem::remove(temporary, ec);
        error = "Файл назначения уже существует и не будет перезаписан.";
        return false;
    }
    std::filesystem::create_hard_link(temporary, destination, ec);
    if (ec) {
        std::error_code cleanup;
        std::filesystem::remove(temporary, cleanup);
        if (std::filesystem::exists(destination, cleanup)) error = "Файл назначения уже существует и не будет перезаписан.";
        else error = "Не удалось сохранить CSV-отчёт: " + ec.message();
        return false;
    }
    std::filesystem::remove(temporary, ec);
    return true;
}

}
