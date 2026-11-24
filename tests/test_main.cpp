#include "CsvStorage.h"
#include "DateTime.h"
#include "ReportService.h"
#include "ReportCsvExporter.h"
#include "HistoryService.h"
#include "ProjectTaskService.h"
#include "TimerController.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const std::string& name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

std::filesystem::path makeTempDirectory(const std::string& name) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path() /
                      ("time-tracker-" + name + "-" + std::to_string(stamp));
    std::filesystem::create_directories(path);
    return path;
}

void testDateTime() {
    timetracker::Timestamp value = 0;
    std::string error;
    check(timetracker::parseDateTime("2024-02-29 12:34:56", value, error), "valid leap date parses");
    check(timetracker::formatDateTime(value) == "2024-02-29 12:34:56", "date formats back");
    check(!timetracker::parseDateTime("2023-02-29 12:34:56", value, error), "invalid leap date rejected");
    check(!timetracker::parseDateTime("2024-13-01 12:34:56", value, error), "invalid month rejected");
    check(timetracker::formatDuration(3661) == "1 ч 01 мин 01 сек", "duration formats");
}

void testTimeEntry() {
    timetracker::TimeEntry entry("20000000-0000-4000-8000-000000000001", "10000000-0000-4000-8000-000000000001", 100);
    check(entry.isActive(), "new entry is active");
    entry.finish(160);
    check(!entry.isActive(), "finished entry is inactive");
    check(entry.durationSeconds() == 60, "finished duration is correct");
    bool rejected = false;
    try {
        entry.edit(200, 200);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "zero-length entry edit rejected");
}

void testUtcStorageAndIntegrity() {
    timetracker::Timestamp timestamp = 0;
    std::string error;
    check(timetracker::parseDateTime("2024-02-29 12:34:56", timestamp, error), "local date parses");
    const auto iso = timetracker::formatUtcIso(timestamp);
    timetracker::Timestamp restored = 0;
    check(timetracker::parseUtcIso(iso, restored) && restored == timestamp, "UTC ISO round-trips");
    const auto directory = makeTempDirectory("integrity");
    timetracker::CsvStorage storage(directory);
    check(storage.saveProjects({{"00000000-0000-4000-8000-000000000001", "Project"}}, error), "integrity project saved");
    check(storage.saveTasks({{"10000000-0000-4000-8000-000000000001", "00000000-0000-4000-8000-000000000001", "Task"}}, error), "integrity task saved");
    check(storage.saveSessions({{"20000000-0000-4000-8000-000000000001", "10000000-0000-4000-8000-000000000001", 100, 200}, {"20000000-0000-4000-8000-000000000002", "10000000-0000-4000-8000-000000000001", 150, 250}}, error), "overlap fixture saved");
    std::vector<timetracker::Project> projects;
    std::vector<timetracker::Task> tasks;
    std::vector<timetracker::TimeEntry> sessions;
    check(!storage.loadAll(projects, tasks, sessions, error), "overlapping sessions rejected");
    std::filesystem::remove_all(directory);
}

void testReport() {
    const std::vector<timetracker::Project> projects{{"00000000-0000-4000-8000-000000000001", "Проект"}, {"00000000-0000-4000-8000-000000000002", "Другой"}};
    const std::vector<timetracker::Task> tasks{{"10000000-0000-4000-8000-000000000001", "00000000-0000-4000-8000-000000000001", "Задача"}, {"10000000-0000-4000-8000-000000000002", "00000000-0000-4000-8000-000000000002", "Вторая"}};
    const std::vector<timetracker::TimeEntry> sessions{
        {"20000000-0000-4000-8000-000000000001", "10000000-0000-4000-8000-000000000001", 90, 110},
        {"20000000-0000-4000-8000-000000000002", "10000000-0000-4000-8000-000000000001", 150, 250},
        {"20000000-0000-4000-8000-000000000003", "10000000-0000-4000-8000-000000000001", 20, 50},
        {"20000000-0000-4000-8000-000000000004", "10000000-0000-4000-8000-000000000001", 120}
    };
    timetracker::ReportService service;
    timetracker::ReportResult result;
    std::string error;
    check(service.build(projects, tasks, sessions, 100, 200, "", result, error), "report builds");
    check(result.rows.size() == 3 && result.rows[0].rowType == "TASK" &&
          result.rows[0].durationSeconds == 60 && result.rows[1].rowType == "PROJECT_TOTAL" &&
          result.rows[1].durationSeconds == 60 && result.rows[2].rowType == "GRAND_TOTAL" &&
          result.rows[2].durationSeconds == 60, "report has task, project and grand totals");
    check(service.build(projects, tasks, sessions, 100, 200, "00000000-0000-4000-8000-000000000001", result, error),
          "report accepts an existing project filter");
    check(result.rows.back().rowType == "GRAND_TOTAL" && result.rows.back().durationSeconds == 60,
          "project-filtered report keeps the selected project's total");
    check(service.build(projects, tasks, {{"20000000-0000-4000-8000-000000000001", "10000000-0000-4000-8000-000000000001", 100, 160}, {"20000000-0000-4000-8000-000000000002", "10000000-0000-4000-8000-000000000002", 160, 180}},
                        100, 200, "", result, error), "multi-project report builds");
    check(result.rows.size() == 5 && result.rows[0].projectName == "Другой" &&
          result.rows[1].rowType == "PROJECT_TOTAL" && result.rows[1].durationSeconds == 20 &&
          result.rows.back().durationSeconds == 80, "report sorts and totals without double counting");
    check(!service.build(projects, tasks, sessions, 200, 100, "", result, error),
          "invalid report interval rejected");
    check(!service.build(projects, tasks, sessions, 100, 200, "missing", result, error),
          "unknown project filter rejected");
}

void testCsvStorage() {
    const auto directory = makeTempDirectory("csv");
    timetracker::CsvStorage storage(directory);
    std::string error;
    std::vector<timetracker::Project> projects;
    check(!storage.saveProjects({{"P-1", "Invalid"}}, error), "malformed UUID rejected");
    check(storage.loadProjects(projects, error), "missing project file initialized");
    check(projects.empty(), "new project table is empty");
    check(storage.saveProjects({{"00000000-0000-4000-8000-000000000001", "Проект, \"А\""}}, error), "project table saved");
    {
        std::ifstream input(directory / "projects.csv", std::ios::binary);
        std::string header;
        std::getline(input, header);
        check(header == "project_id,name", "project file uses specification header");
    }
    check(storage.loadProjects(projects, error), "project table loaded");
    check(projects.size() == 1 && projects.front().name() == "Проект, \"А\"",
          "quoted CSV field round-trips");
    check(storage.saveTasks({{"10000000-0000-4000-8000-000000000001", "00000000-0000-4000-8000-000000000001", "Задача"}}, error), "task table saved");
    {
        std::ifstream input(directory / "tasks.csv", std::ios::binary);
        std::string header;
        std::getline(input, header);
        check(header == "task_id,project_id,name", "task file uses specification header");
    }
    check(storage.saveSessions({{"30000000-0000-4000-8000-000000000001", "10000000-0000-4000-8000-000000000001", 0, 1}}, error), "session table saved");
    {
        std::ifstream input(directory / "sessions.csv", std::ios::binary);
        std::string header, row;
        std::getline(input, header);
        std::getline(input, row);
        check(header == "entry_id,task_id,started_at_utc,ended_at_utc", "session file uses specification header");
        check(row.find("1970-01-01T00:00:00Z") != std::string::npos, "session timestamps are stored as UTC ISO");
    }
    {
        std::ofstream output(directory / "tasks.csv", std::ios::binary);
        output << "wrong,header\n";
    }
    std::vector<timetracker::Task> tasks;
    check(!storage.loadTasks(tasks, error), "invalid CSV header rejected");
    timetracker::ReportCsvExporter exporter(directory);
    timetracker::ReportResult report;
    report.rows.push_back({"TASK", "00000000-0000-4000-8000-000000000001", "Проект", "10000000-0000-4000-8000-000000000001", "Задача, \"А\"", 42});
    check(!exporter.write(report, directory / "projects.csv", error),
          "export cannot overwrite source CSV");
    check(exporter.write(report, directory / "report.csv", error), "new report file written");
    check(!exporter.write(report, directory / "report.csv", error), "existing export is not overwritten");
    check(!exporter.write(report, directory / "nested" / ".." / "projects.csv", error),
          "normalized path cannot overwrite source CSV");
    {
        std::ifstream input(directory / "report.csv", std::ios::binary);
        const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        check(content.find("row_type,project_id,project_name,task_id,task_name,total_seconds\r\n") == 0,
              "export uses required report header");
        check(content.find("Задача, \"\"А\"\"") != std::string::npos,
              "export escapes quotes and commas");
    }
    std::filesystem::remove_all(directory);
}

void testProjectTaskAndHistoryServices() {
    const auto directory = makeTempDirectory("services");
    timetracker::CsvStorage storage(directory);
    timetracker::ProjectTaskService projectTasks(storage);
    std::string error;
    timetracker::Project project;
    timetracker::Task task;
    check(projectTasks.createProject("Проект", project, error), "project service creates project");
    check(project.id().size() == 36 && project.id()[14] == '4', "project ID is UUID v4");
    check(projectTasks.createTask(project.id(), "Задача", task, error), "project service creates task");
    check(!projectTasks.createTask("missing", "Задача", task, error), "invalid project reference rejected");
    check(storage.saveSessions({{"30000000-0000-4000-8000-000000000001", task.id(), 100, 200}, {"30000000-0000-4000-8000-000000000002", task.id(), 300, 400}}, error),
          "history fixture saved");
    timetracker::HistoryService history(storage);
    std::vector<timetracker::HistoryRecord> records;
    check(history.list(project.id(), 190, 250, records, error), "history intersection filter succeeds");
    check(records.size() == 1 && records[0].session.durationSeconds() == 100,
          "history includes full intersecting session duration");
    timetracker::TimeEntry updated;
    check(!history.edit("30000000-0000-4000-8000-000000000001", 250, 350, 500, updated, error), "overlapping edit rejected");
    check(history.edit("30000000-0000-4000-8000-000000000001", 50, 90, 500, updated, error), "valid historical edit saved");
    check(updated.startedAt() == 50 && updated.durationSeconds() == 40, "edited interval values retained");
    std::filesystem::remove_all(directory);
}

void testTimerController() {
    const auto directory = makeTempDirectory("timer");
    timetracker::CsvStorage storage(directory);
    std::string error;
    check(storage.saveProjects({{"00000000-0000-4000-8000-000000000001", "Проект"}}, error), "timer project fixture saved");
    check(storage.saveTasks({{"10000000-0000-4000-8000-000000000001", "00000000-0000-4000-8000-000000000001", "Задача"}}, error), "timer task fixture saved");
    check(storage.saveSessions({}, error), "timer session fixture saved");
    timetracker::Timestamp fakeNow = 1000;
    timetracker::TimerController controller(storage, [&fakeNow] { return fakeNow; });
    check(controller.start("10000000-0000-4000-8000-000000000001").success, "timer starts for existing task");
    check(!controller.start("10000000-0000-4000-8000-000000000001").success, "second active timer is rejected");
    timetracker::TimeEntry entry;
    timetracker::Task task;
    timetracker::Project project;
    check(controller.getActive(entry, task, project, error), "active timer can be loaded");
    check(entry.isActive() && task.id() == "10000000-0000-4000-8000-000000000001" && project.id() == "00000000-0000-4000-8000-000000000001",
          "active timer links task and project");
    timetracker::CsvStorage restartedStorage(directory);
    timetracker::TimerController restartedController(restartedStorage, [&fakeNow] { return fakeNow; });
    check(restartedController.getActive(entry, task, project, error), "active timer survives controller restart");
    check(!controller.stop().success, "zero-second stop is rejected");
    fakeNow = 1060;
    check(controller.stop().success, "timer stops when clock advances");
    fakeNow = 1059;
    check(!controller.start("10000000-0000-4000-8000-000000000001").success, "start before prior session end is rejected");
    std::filesystem::remove_all(directory);
}

}

int main() {
    testDateTime();
    testTimeEntry();
    testUtcStorageAndIntegrity();
    testReport();
    testCsvStorage();
    testProjectTaskAndHistoryServices();
    testTimerController();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}
