import csv
from pathlib import Path
import subprocess
import sys
import tempfile
import time

exe = str(Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory(prefix="time-tracker-cli-") as directory:
    root = Path(directory)
    data = root / "data"
    def run(commands):
        completed = subprocess.run([exe, str(data)], input=commands, text=True,
                                   encoding="utf-8", capture_output=True, cwd=root, timeout=20)
        assert completed.returncode == 0, completed.stderr
        return completed.stdout
    def rows(name):
        with (data / name).open(encoding="utf-8", newline="") as stream:
            return list(csv.DictReader(stream))

    output = run('10\n1\nУчеба, "А"\n0\n')
    assert "Сначала сформируйте отчет" in output, output
    project = rows("projects.csv")[0]
    assert project["name"] == 'Учеба, "А"'
    run("2\n" + project["project_id"] + "\nПодготовка ПР2\n0\n")
    task = rows("tasks.csv")[0]
    output = run("4\n" + task["task_id"] + "\n6\n4\n" + task["task_id"] + "\n0\n")
    assert "Уже есть активная сессия" in output and task["task_id"] in output, output
    session = rows("sessions.csv")[0]
    assert not session["ended_at_utc"]
    time.sleep(1.1)
    output = run("6\n5\n7\n\n\n\n0\n")
    assert "Активная задача" in output and "Учет остановлен" in output, output
    assert rows("sessions.csv")[0]["entry_id"] == session["entry_id"]
    assert rows("sessions.csv")[0]["ended_at_utc"]
    report = root / "reports" / "snapshot.csv"
    output = run("9\n" + session["entry_id"] + "\n2024-11-25 10:00:00\n2024-11-25 10:30:00\n"
                 "8\n2024-11-25 10:15:00\n2024-11-25 10:45:00\n\n"
                 "9\n" + session["entry_id"] + "\n2024-11-25 09:00:00\n2024-11-25 09:10:00\n"
                 "10\n" + str(report) + "\n10\n" + str(report) + "\n0\n")
    assert "Отчет сохранен" in output and "не будет перезаписан" in output, output
    with report.open(encoding="utf-8", newline="") as stream:
        exported = list(csv.DictReader(stream))
    assert [r["row_type"] for r in exported] == ["TASK", "PROJECT_TOTAL", "GRAND_TOTAL"]
    assert all(r["total_seconds"] == "900" for r in exported), exported
    assert exported[0]["project_name"] == project["name"]
    source_before = (data / "sessions.csv").read_bytes()
    output = run("9\n" + session["entry_id"] + "\n2024-11-25 10:00:00\n2024-11-25 09:00:00\n0\n")
    assert "Запись не изменена" in output, output
    assert (data / "sessions.csv").read_bytes() == source_before
    output = run("8\nдень\n\n8\nнеделя\n\n8\nмесяц\n\n0\n")
    assert output.count("Отчет за период") == 3, output
    assert "GRAND_TOTAL" in output
print("CLI smoke test passed: restart, timer, history, editing, report snapshot, CSV export")
