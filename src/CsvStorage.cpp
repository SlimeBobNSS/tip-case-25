#include "CsvStorage.h"
#include "DateTime.h"
#include "IdGenerator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <system_error>

namespace timetracker {
namespace {

std::string encodeField(const std::string& value) {
    if (value.find_first_of(",\"\r\n") == std::string::npos) {
        return value;
    }
    std::string result = "\"";
    for (char ch : value) {
        if (ch == '"') {
            result += "\"\"";
        } else {
            result += ch;
        }
    }
    result += '"';
    return result;
}

std::string encodeRow(const std::vector<std::string>& row) {
    std::ostringstream out;
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i != 0) {
            out << ',';
        }
        out << encodeField(row[i]);
    }
    return out.str();
}

bool parseRow(const std::string& line, std::vector<std::string>& fields) {
    fields.clear();
    std::string field;
    bool quoted = false;
    bool quoteClosed = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (quoted) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    quoteClosed = true;
                }
            } else {
                field += ch;
            }
        } else if (ch == ',' ) {
            fields.push_back(field);
            field.clear();
            quoteClosed = false;
        } else if (ch == '"' && field.empty() && !quoteClosed) {
            quoted = true;
        } else if (ch == '"') {
            return false;
        } else if (quoteClosed && ch != '\r') {
            return false;
        } else {
            field += ch;
        }
    }
    if (quoted) {
        return false;
    }
    fields.push_back(field);
    return true;
}

bool validUtf8(const std::string& value) {
    for (std::size_t i = 0; i < value.size();) {
        const auto c = static_cast<unsigned char>(value[i]);
        std::size_t count = c <= 0x7f ? 1 : (c >= 0xc2 && c <= 0xdf ? 2 :
                            (c >= 0xe0 && c <= 0xef ? 3 : (c >= 0xf0 && c <= 0xf4 ? 4 : 0)));
        if (count == 0 || i + count > value.size()) return false;
        for (std::size_t j = 1; j < count; ++j) {
            if ((static_cast<unsigned char>(value[i + j]) & 0xc0) != 0x80) return false;
        }
        if (count == 3) {
            const auto second = static_cast<unsigned char>(value[i + 1]);
            if ((c == 0xe0 && second < 0xa0) || (c == 0xed && second >= 0xa0)) return false;
        }
        if (count == 4) {
            const auto second = static_cast<unsigned char>(value[i + 1]);
            if ((c == 0xf0 && second < 0x90) || (c == 0xf4 && second >= 0x90)) return false;
        }
        i += count;
    }
    return true;
}

bool validName(const std::string& name) {
    if (name.empty() || name.find_first_of("\r\n") != std::string::npos ||
        std::isspace(static_cast<unsigned char>(name.front())) ||
        std::isspace(static_cast<unsigned char>(name.back()))) return false;
    return validUtf8(name);
}

bool replaceFile(const std::filesystem::path& temporary,
                 const std::filesystem::path& destination, std::string& error) {
    std::error_code ec;
    std::filesystem::rename(temporary, destination, ec);
    if (!ec) {
        return true;
    }

    const auto backup = destination.string() + ".bak";
    std::error_code backupError;
    const bool hadDestination = std::filesystem::exists(destination, backupError) && !backupError;
    if (hadDestination) {
        std::filesystem::remove(backup, backupError);
        backupError.clear();
        std::filesystem::rename(destination, backup, backupError);
        if (backupError) {
            error = "Не удалось подготовить замену файла " + destination.string() + ": " + backupError.message();
            return false;
        }
    }

    ec.clear();
    std::filesystem::rename(temporary, destination, ec);
    if (ec) {
        if (hadDestination) {
            std::error_code restoreError;
            std::filesystem::rename(backup, destination, restoreError);
        }
        error = "Не удалось сохранить файл " + destination.string() + ": " + ec.message();
        return false;
    }
    if (hadDestination) {
        std::filesystem::remove(backup, backupError);
    }
    return true;
}

}

CsvStorage::CsvStorage(std::filesystem::path directory) : directory_(std::move(directory)) {}

bool CsvStorage::readTable(const std::string& filename, const std::vector<std::string>& header,
                           std::vector<std::vector<std::string>>& rows, std::string& error) {
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    if (ec) {
        error = "Не удалось создать каталог данных: " + ec.message();
        return false;
    }
    const auto path = directory_ / filename;
    if (!std::filesystem::exists(path)) {
        return writeTable(filename, header, {}, error);
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "Не удалось открыть файл " + path.string() + " для чтения.";
        return false;
    }
    std::string line;
    if (!std::getline(input, line)) {
        error = "Файл " + path.string() + " пуст или не содержит заголовка.";
        return false;
    }
    if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    std::vector<std::string> parsed;
    if (!parseRow(line, parsed) || parsed != header) {
        error = "Заголовок файла " + path.string() + " имеет неверный формат.";
        return false;
    }

    rows.clear();
    std::size_t lineNumber = 1;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (!parseRow(line, parsed) || parsed.size() != header.size()) {
            error = "Некорректная CSV-строка " + std::to_string(lineNumber) + " в файле " + path.string() + ".";
            return false;
        }
        for (const auto& field : parsed) {
            if (!validUtf8(field)) {
                error = "CSV-файл содержит данные, не являющиеся корректным UTF-8.";
                return false;
            }
        }
        rows.push_back(parsed);
    }
    if (input.bad()) {
        error = "Ошибка при чтении файла " + path.string() + ".";
        return false;
    }
    return true;
}

bool CsvStorage::writeTable(const std::string& filename, const std::vector<std::string>& header,
                            const std::vector<std::vector<std::string>>& rows, std::string& error) const {
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    if (ec) {
        error = "Не удалось создать каталог данных: " + ec.message();
        return false;
    }
    const auto path = directory_ / filename;
    const auto temporary = path.string() + ".tmp";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) {
        error = "Не удалось открыть временный файл " + temporary + " для записи.";
        return false;
    }
    output << encodeRow(header) << '\n';
    for (const auto& row : rows) {
        if (row.size() != header.size()) {
            error = "Внутренняя ошибка: число полей CSV-строки не совпадает с заголовком.";
            return false;
        }
        output << encodeRow(row) << '\n';
    }
    output.flush();
    if (!output) {
        error = "Ошибка записи временного файла " + temporary + ".";
        return false;
    }
    output.close();
    if (!output) {
        error = "Не удалось закрыть временный файл " + temporary + ".";
        return false;
    }
    if (!replaceFile(temporary, path, error)) {
        return false;
    }
    return true;
}

bool CsvStorage::loadProjects(std::vector<Project>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    if (!readTable("projects.csv", {"project_id", "name"}, rows, error)) {
        return false;
    }
    items.clear();
    for (const auto& row : rows) {
        if (row[0].empty() || row[1].empty()) {
            error = "Файл projects.csv содержит пустой идентификатор или название.";
            return false;
        }
        items.emplace_back(row[0], row[1]);
    }
    return true;
}

bool CsvStorage::saveProjects(const std::vector<Project>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& item : items) {
        if (!isUuid(item.id()) || !validName(item.name())) {
            error = "Нельзя сохранить проект с некорректным UUID или названием.";
            return false;
        }
        rows.push_back({item.id(), item.name()});
    }
    return writeTable("projects.csv", {"project_id", "name"}, rows, error);
}

bool CsvStorage::loadTasks(std::vector<Task>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    if (!readTable("tasks.csv", {"task_id", "project_id", "name"}, rows, error)) {
        return false;
    }
    items.clear();
    for (const auto& row : rows) {
        if (row[0].empty() || row[1].empty() || row[2].empty()) {
            error = "Файл tasks.csv содержит некорректные данные.";
            return false;
        }
        items.emplace_back(row[0], row[1], row[2]);
    }
    return true;
}

bool CsvStorage::saveTasks(const std::vector<Task>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& item : items) {
        if (!isUuid(item.id()) || !isUuid(item.projectId()) || !validName(item.name())) {
            error = "Нельзя сохранить задачу с некорректным UUID или названием.";
            return false;
        }
        rows.push_back({item.id(), item.projectId(), item.name()});
    }
    return writeTable("tasks.csv", {"task_id", "project_id", "name"}, rows, error);
}

bool CsvStorage::loadSessions(std::vector<TimeEntry>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    if (!readTable("sessions.csv", {"entry_id", "task_id", "started_at_utc", "ended_at_utc"}, rows, error)) {
        return false;
    }
    items.clear();
    for (const auto& row : rows) {
        Timestamp startedAt = 0;
        Timestamp endedAt = 0;
        if (row[0].empty() || row[1].empty() || !parseUtcIso(row[2], startedAt)) {
            error = "Файл sessions.csv содержит некорректную запись.";
            return false;
        }
        std::optional<Timestamp> end;
        if (!row[3].empty()) {
            if (!parseUtcIso(row[3], endedAt) || endedAt <= startedAt) {
                error = "Файл sessions.csv содержит некорректное время окончания.";
                return false;
            }
            end = endedAt;
        }
        items.emplace_back(row[0], row[1], startedAt, end);
    }
    return true;
}

bool CsvStorage::saveSessions(const std::vector<TimeEntry>& items, std::string& error) {
    std::vector<std::vector<std::string>> rows;
    for (const auto& item : items) {
        if (!isUuid(item.id()) || !isUuid(item.taskId())) {
            error = "Нельзя сохранить сессию с некорректным ID или ссылкой на задачу.";
            return false;
        }
        rows.push_back({item.id(), item.taskId(), formatUtcIso(item.startedAt()),
                        item.endedAt() ? formatUtcIso(*item.endedAt()) : ""});
    }
    return writeTable("sessions.csv", {"entry_id", "task_id", "started_at_utc", "ended_at_utc"}, rows, error);
}

bool CsvStorage::loadAll(std::vector<Project>& projects, std::vector<Task>& tasks,
                         std::vector<TimeEntry>& sessions, std::string& error) {
    if (!loadProjects(projects, error) || !loadTasks(tasks, error) || !loadSessions(sessions, error)) return false;
    std::set<std::string> projectIds, taskIds, entryIds;
    for (const auto& project : projects) {
        if (!isUuid(project.id()) || !validName(project.name()) ||
            !projectIds.insert(project.id()).second) {
            error = "Файл projects.csv содержит некорректный или повторный UUID либо некорректное название.";
            return false;
        }
    }
    for (const auto& task : tasks) {
        if (!isUuid(task.id()) || !validName(task.name()) ||
            !taskIds.insert(task.id()).second || projectIds.count(task.projectId()) == 0) {
            error = "Файл tasks.csv содержит некорректный или повторный UUID, название либо ссылку на отсутствующий проект.";
            return false;
        }
    }
    int activeCount = 0;
    std::vector<const TimeEntry*> ordered;
    for (const auto& session : sessions) {
        if (!isUuid(session.id()) || taskIds.count(session.taskId()) == 0 ||
            !entryIds.insert(session.id()).second) {
            error = "Файл sessions.csv содержит некорректный или повторный UUID либо ссылку на отсутствующую задачу.";
            return false;
        }
        if (session.isActive()) ++activeCount;
        ordered.push_back(&session);
    }
    if (activeCount > 1) {
        error = "Файл sessions.csv содержит более одной активной сессии.";
        return false;
    }
    std::sort(ordered.begin(), ordered.end(), [](const TimeEntry* a, const TimeEntry* b) {
        return a->startedAt() < b->startedAt();
    });
    Timestamp previousEnd = -1;
    for (const auto* session : ordered) {
        if (session->startedAt() < previousEnd) {
            error = "Файл sessions.csv содержит пересекающиеся сессии.";
            return false;
        }
        if (session->endedAt()) previousEnd = *session->endedAt();
        else previousEnd = std::numeric_limits<Timestamp>::max();
    }
    return true;
}

const std::filesystem::path& CsvStorage::directory() const { return directory_; }

}
