#include "CsvStorage.h"
#include "DateTime.h"
#include "ReportService.h"
#include "ReportCsvExporter.h"
#include "ProjectTaskService.h"
#include "HistoryService.h"
#include "TimerController.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace timetracker;

namespace {

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string{};
}

std::string prompt(const std::string& text) {
    std::cout << text;
    std::string value;
    std::getline(std::cin, value);
    return trim(value);
}

int readMenuChoice() {
    const std::string input = prompt("Выберите пункт: ");
    try {
        std::size_t used = 0;
        const int value = std::stoi(input, &used);
        return used == input.size() ? value : -1;
    } catch (...) {
        return -1;
    }
}

bool loadAll(CsvStorage& storage, std::vector<Project>& projects,
             std::vector<Task>& tasks, std::vector<TimeEntry>& sessions,
             std::string& error) {
    return storage.loadAll(projects, tasks, sessions, error);
}

void printProjects(const std::vector<Project>& projects) {
    if (projects.empty()) {
        std::cout << "Проектов пока нет.\n";
        return;
    }
    std::cout << "Проекты:\n";
    for (const auto& project : projects) {
        std::cout << "  " << project.id() << " — " << project.name() << '\n';
    }
}

void printTasks(const std::vector<Project>& projects, const std::vector<Task>& tasks) {
    if (tasks.empty()) {
        std::cout << "Задач пока нет.\n";
        return;
    }
    std::cout << "Задачи:\n";
    for (const auto& task : tasks) {
        const auto project = std::find_if(projects.begin(), projects.end(), [&](const Project& item) {
            return item.id() == task.projectId();
        });
        const std::string projectName = project == projects.end() ? "[проект не найден]" : project->name();
        std::cout << "  " << task.id() << " — " << task.name() << " | проект: " << projectName
                  << '\n';
    }
}

bool readDate(const std::string& label, Timestamp& value) {
    const std::string text = prompt(label + " (YYYY-MM-DD HH:MM:SS): ");
    std::string error;
    if (!parseDateTime(text, value, error)) {
        std::cout << "Ошибка: " << error << '\n';
        return false;
    }
    return true;
}

bool tryReadOptionalDate(const std::string& label, std::optional<Timestamp>& value) {
    const std::string text = prompt(label + " (Enter — без ограничения, иначе YYYY-MM-DD HH:MM:SS): ");
    if (text.empty()) {
        value.reset();
        return true;
    }
    Timestamp parsed = 0;
    std::string error;
    if (!parseDateTime(text, parsed, error)) {
        std::cout << "Ошибка: " << error << '\n';
        return false;
    }
    value = parsed;
    return true;
}

void createProject(CsvStorage& storage) {
    const auto name = prompt("Название проекта: ");
    ProjectTaskService service(storage);
    Project created;
    std::string error;
    if (!service.createProject(name, created, error)) {
        std::cout << "Не удалось создать проект: " << error << '\n';
        return;
    }
    std::cout << "Проект создан: " << created.id() << " — " << created.name() << '\n';
}

void createTask(CsvStorage& storage) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::string error;
    ProjectTaskService service(storage);
    if (!service.list(projects, tasks, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    if (projects.empty()) {
        std::cout << "Сначала создайте проект.\n";
        return;
    }
    printProjects(projects);
    const auto projectId = prompt("ID проекта для новой задачи (UUID, см. список выше): ");
    const auto name = prompt("Название задачи: ");
    Task created;
    if (!service.createTask(projectId, name, created, error)) {
        std::cout << "Не удалось создать задачу: " << error << '\n';
        return;
    }
    std::cout << "Задача создана: " << created.id() << " — " << created.name() << '\n';
}

void showProjectsAndTasks(CsvStorage& storage) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!storage.loadAll(projects, tasks, sessions, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    printProjects(projects);
    printTasks(projects, tasks);
}

void startTimer(TimerController& timer, CsvStorage& storage) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!loadAll(storage, projects, tasks, sessions, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    printTasks(projects, tasks);
    const auto taskId = prompt("ID задачи для запуска таймера (UUID, см. список выше): ");
    const auto result = timer.start(taskId);
    std::cout << (result.success ? "Успешно: " : "Отказ: ") << result.message << '\n';
}

void stopTimer(TimerController& timer) {
    const auto result = timer.stop();
    std::cout << (result.success ? "Успешно: " : "Отказ: ") << result.message << '\n';
}

void showActiveTimer(TimerController& timer) {
    TimeEntry entry;
    Task task;
    Project project;
    std::string error;
    if (!timer.getActive(entry, task, project, error)) {
        std::cout << error << '\n';
        return;
    }
    std::cout << "ID задачи: " << task.id() << "\nАктивная задача: " << task.name() << " (" << project.name() << ")\n"
              << "Начало: " << formatDateTime(entry.startedAt()) << "\n"
              << "Прошло: " << formatDuration(now() - entry.startedAt()) << '\n';
}

void showHistory(CsvStorage& storage) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!loadAll(storage, projects, tasks, sessions, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    printProjects(projects);
    const std::string projectFilter = prompt("Фильтр по ID проекта (Enter — все): ");
    std::optional<Timestamp> from, to;
    if (!tryReadOptionalDate("Начало диапазона", from) || !tryReadOptionalDate("Конец диапазона", to)) return;
    std::vector<HistoryRecord> records;
    HistoryService service(storage);
    if (!service.list(projectFilter, from, to, records, error)) {
        std::cout << "Ошибка: " << error << '\n';
        return;
    }
    if (records.empty()) {
        std::cout << "Подходящих завершенных записей нет.\n";
        return;
    }
    for (const auto& record : records) {
        const auto& session = record.session;
        std::cout << session.id() << " | " << record.project.name() << " / " << record.task.name()
                  << " | " << formatDateTime(session.startedAt()) << " — "
                  << formatDateTime(*session.endedAt()) << " | "
                  << formatDuration(*session.durationSeconds()) << '\n';
    }
}

void editSession(CsvStorage& storage) {
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!loadAll(storage, projects, tasks, sessions, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    bool anyCompleted = false;
    std::cout << "Завершенные сессии:\n";
    for (const auto& session : sessions) {
        if (session.isActive()) continue;
        anyCompleted = true;
        std::cout << "  " << session.id() << " | " << session.taskId() << " | "
                  << formatDateTime(session.startedAt()) << " — "
                  << formatDateTime(*session.endedAt()) << '\n';
    }
    if (!anyCompleted) {
        std::cout << "Завершенных сессий пока нет.\n";
        return;
    }
    const auto id = prompt("ID сессии для редактирования (UUID из списка выше): ");
    Timestamp start = 0, end = 0;
    if (!readDate("Новое время начала", start) || !readDate("Новое время окончания", end)) return;
    TimeEntry updated;
    HistoryService service(storage);
    if (!service.edit(id, start, end, now(), updated, error)) {
        std::cout << "Запись не изменена: " << error << '\n';
        return;
    }
    std::cout << "Запись " << id << " обновлена. Длительность: "
              << formatDuration(*updated.durationSeconds()) << '\n';
}

bool calendarPeriod(const std::string& kind, Timestamp& start, Timestamp& end) {
    const std::time_t current = static_cast<std::time_t>(now());
    const auto* localPointer = std::localtime(&current);
    if (!localPointer) return false;
    std::tm local = *localPointer;
    local.tm_hour = 0;
    local.tm_min = 0;
    local.tm_sec = 0;
    if (kind == "неделя") {
        local.tm_mday -= (local.tm_wday + 6) % 7;
    } else if (kind == "месяц") {
        local.tm_mday = 1;
    }
    local.tm_isdst = -1;
    const std::time_t first = std::mktime(&local);
    if (first == static_cast<std::time_t>(-1)) return false;
    std::tm next = local;
    if (kind == "день") next.tm_mday += 1;
    else if (kind == "неделя") next.tm_mday += 7;
    else next.tm_mon += 1;
    next.tm_isdst = -1;
    const std::time_t after = std::mktime(&next);
    if (after == static_cast<std::time_t>(-1) || after <= first) return false;
    start = static_cast<Timestamp>(first);
    end = static_cast<Timestamp>(after);
    return true;
}

void createReport(CsvStorage& storage, std::optional<Report>& lastReport) {
    Timestamp start = 0, end = 0;
    const auto firstInput = prompt("Начало периода (YYYY-MM-DD HH:MM:SS, либо день/неделя/месяц): ");
    if (firstInput == "день" || firstInput == "неделя" || firstInput == "месяц") {
        if (!calendarPeriod(firstInput, start, end)) {
            std::cout << "Не удалось определить календарные границы периода.\n";
            return;
        }
    } else {
        std::string dateError;
        if (!parseDateTime(firstInput, start, dateError)) {
            std::cout << "Ошибка: " << dateError << '\n';
            return;
        }
        if (!readDate("Конец периода", end)) return;
    }
    if (end <= start) {
        std::cout << "Конец периода должен быть позже начала.\n";
        return;
    }
    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!loadAll(storage, projects, tasks, sessions, error)) {
        std::cout << "Ошибка чтения данных: " << error << '\n';
        return;
    }
    printProjects(projects);
    const auto projectId = prompt("Фильтр по проекту (Enter — все проекты): ");
    ReportService service(storage);
    auto built = service.build({start, end}, projectId.empty() ? std::nullopt : std::optional<std::string>(projectId));
    if (!built) {
        std::cout << "Не удалось построить отчет: " << built.error << '\n';
        return;
    }
    lastReport = std::move(*built.value);
    const Report& result = *lastReport;
    std::cout << "\nОтчет за период " << formatDateTime(start) << " — " << formatDateTime(end) << '\n';
    std::cout << "Тип | ID проекта | Проект | ID задачи | Задача | Всего секунд\n";
    for (const auto& row : result.rows()) {
        std::cout << row.rowType << " | " << row.projectId << " | " << row.projectName << " | "
                  << row.taskId << " | " << row.taskName << " | " << row.durationSeconds << '\n';
    }
}

void exportReport(CsvStorage& storage, const std::optional<Report>& lastReport) {
    if (!lastReport) {
        std::cout << "Сначала сформируйте отчет (пункт 8).\n";
        return;
    }
    const Report& result = *lastReport;
    std::string error;
    auto file = prompt("Путь к CSV-файлу или каталогу (Enter — data/reports): ");
    if (file.empty()) file = "data/reports";
    std::filesystem::path exportPath = std::filesystem::u8path(file);
    std::error_code pathError;
    const bool pathExists = std::filesystem::exists(exportPath, pathError);
    const bool pathIsDirectory = !pathError && pathExists && std::filesystem::is_directory(exportPath, pathError);
    const bool directoryRequest = pathIsDirectory || (!pathExists &&
        ((!file.empty() && (file.back() == '/' || file.back() == '\\')) || exportPath.extension() != ".csv"));
    if (directoryRequest) {
        const auto displayed = formatDateTime(now());
        std::string stamp;
        for (char ch : displayed) if (std::isdigit(static_cast<unsigned char>(ch))) stamp += ch;
        if (stamp.size() == 14) stamp.insert(8, "_");
        exportPath /= "report_" + stamp + ".csv";
    }
    ReportCsvExporter exporter(storage.directory());
    if (!exporter.write(result, exportPath, error)) {
        std::cout << "Не удалось сохранить CSV: " << error << '\n';
        return;
    }
    std::cout << "Отчет сохранен: " << std::filesystem::absolute(exportPath).string() << '\n';
}

void printMenu() {
    std::cout << "\n=== Тайм-трекер ===\n"
              << "1. Создать проект\n"
              << "2. Создать задачу\n"
              << "3. Показать проекты и задачи\n"
              << "4. Запустить таймер\n"
              << "5. Остановить таймер\n"
              << "6. Показать активную сессию\n"
              << "7. История с фильтрами\n"
              << "8. Сформировать отчет\n"
              << "9. Редактировать завершенную сессию\n"
              << "10. Экспортировать последний сформированный отчет\n"
              << "0. Выход\n";
}

}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::optional<Report> lastReport;
    std::filesystem::path dataDirectory = argc > 1 ? argv[1] : "data";
    CsvStorage storage(dataDirectory);
    TimerController timer(storage);

    std::vector<Project> projects;
    std::vector<Task> tasks;
    std::vector<TimeEntry> sessions;
    std::string error;
    if (!loadAll(storage, projects, tasks, sessions, error)) {
        std::cout << "Внимание: при чтении данных возникла ошибка: " << error << '\n'
                  << "Пункты, которым нужны эти данные, покажут ошибку; исходные файлы не перезаписаны.\n";
    } else {
        std::cout << "Данные хранятся в: " << std::filesystem::absolute(dataDirectory).string() << '\n';
    }

    while (std::cin) {
        printMenu();
        const int choice = readMenuChoice();
        if (!std::cin) break;
        switch (choice) {
            case 1: createProject(storage); break;
            case 2: createTask(storage); break;
            case 3: showProjectsAndTasks(storage); break;
            case 4: startTimer(timer, storage); break;
            case 5: stopTimer(timer); break;
            case 6: showActiveTimer(timer); break;
            case 7: showHistory(storage); break;
            case 8: createReport(storage, lastReport); break;
            case 10: exportReport(storage, lastReport); break;
            case 9: editSession(storage); break;
            case 0: std::cout << "До свидания!\n"; return 0;
            default: std::cout << "Нет такого пункта. Введите число от 0 до 10.\n"; break;
        }
    }
    return 0;
}
