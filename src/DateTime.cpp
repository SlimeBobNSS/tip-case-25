#include "DateTime.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace timetracker {
namespace {

bool readPart(const std::string& text, std::size_t position, std::size_t length, int& value) {
    if (position + length > text.size()) {
        return false;
    }
    for (std::size_t i = position; i < position + length; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
    }
    value = std::stoi(text.substr(position, length));
    return true;
}

bool isLeapYear(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2) {
        return isLeapYear(year) ? 29 : 28;
    }
    return month >= 1 && month <= 12 ? days[month - 1] : 0;
}

std::tm localTime(std::time_t value) {
    const auto* valuePointer = std::localtime(&value);
    return valuePointer ? *valuePointer : std::tm{};
}

std::tm utcTime(std::time_t value) {
    const auto* valuePointer = std::gmtime(&value);
    return valuePointer ? *valuePointer : std::tm{};
}

std::time_t utcTimestamp(std::tm value) {
    long long year = static_cast<long long>(value.tm_year) + 1900;
    const long long month = value.tm_mon + 1;
    year -= month <= 2;
    const long long era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned adjustedMonth = static_cast<unsigned>(month + (month > 2 ? -3 : 9));
    const unsigned dayOfYear = (153 * adjustedMonth + 2) / 5 + static_cast<unsigned>(value.tm_mday - 1);
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    const long long days = era * 146097 + static_cast<long long>(dayOfEra) - 719468;
    return static_cast<std::time_t>(days * 86400 + value.tm_hour * 3600 + value.tm_min * 60 + value.tm_sec);
}

}

Timestamp now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool parseDateTime(const std::string& text, Timestamp& result, std::string& error) {
    if (text.size() != 19 || text[4] != '-' || text[7] != '-' ||
        text[10] != ' ' || text[13] != ':' || text[16] != ':') {
        error = "Ожидается формат YYYY-MM-DD HH:MM:SS.";
        return false;
    }

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (!readPart(text, 0, 4, year) || !readPart(text, 5, 2, month) ||
        !readPart(text, 8, 2, day) || !readPart(text, 11, 2, hour) ||
        !readPart(text, 14, 2, minute) || !readPart(text, 17, 2, second)) {
        error = "Дата и время должны содержать только цифры в формате YYYY-MM-DD HH:MM:SS.";
        return false;
    }
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month) ||
        hour > 23 || minute > 59 || second > 59) {
        error = "Указана некорректная дата или время.";
        return false;
    }

    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    std::tm standard = tm;
    std::tm daylight = tm;
    standard.tm_isdst = 0;
    daylight.tm_isdst = 1;
    const std::time_t standardTime = std::mktime(&standard);
    const std::time_t daylightTime = std::mktime(&daylight);
    auto matchesInput = [&](std::time_t candidate) {
        if (candidate == static_cast<std::time_t>(-1)) return false;
        const auto roundTrip = localTime(candidate);
        return roundTrip.tm_year == year - 1900 && roundTrip.tm_mon == month - 1 &&
               roundTrip.tm_mday == day && roundTrip.tm_hour == hour &&
               roundTrip.tm_min == minute && roundTrip.tm_sec == second;
    };
    const bool standardMatches = matchesInput(standardTime);
    const bool daylightMatches = matchesInput(daylightTime);
    if (!standardMatches && !daylightMatches) {
        error = "Указанное локальное время не существует в часовом поясе устройства.";
        return false;
    }
    if (standardMatches && daylightMatches && standardTime != daylightTime) {
        error = "Указанное локальное время неоднозначно в часовом поясе устройства.";
        return false;
    }
    const std::time_t converted = standardMatches ? standardTime : daylightTime;
    if (converted == static_cast<std::time_t>(-1)) {
        error = "Не удалось преобразовать дату и время.";
        return false;
    }
    result = static_cast<Timestamp>(converted);
    return true;
}

std::string formatUtcIso(Timestamp value) {
    const std::time_t raw = static_cast<std::time_t>(value);
    const std::tm tm = utcTime(raw);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bool parseUtcIso(const std::string& text, Timestamp& result) {
    if (text.size() != 20 || text[4] != '-' || text[7] != '-' || text[10] != 'T' ||
        text[13] != ':' || text[16] != ':' || text[19] != 'Z') return false;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (!readPart(text, 0, 4, year) || !readPart(text, 5, 2, month) ||
        !readPart(text, 8, 2, day) || !readPart(text, 11, 2, hour) ||
        !readPart(text, 14, 2, minute) || !readPart(text, 17, 2, second) ||
        year < 1970 || month < 1 || month > 12 || day < 1 || day > daysInMonth(year, month) ||
        hour > 23 || minute > 59 || second > 59) return false;
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    const std::time_t converted = utcTimestamp(tm);
    if (converted == static_cast<std::time_t>(-1)) return false;
    result = static_cast<Timestamp>(converted);
    return formatUtcIso(result) == text;
}

std::string formatDateTime(Timestamp value) {
    const std::time_t raw = static_cast<std::time_t>(value);
    const std::tm tm = localTime(raw);
    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

std::string formatDuration(Timestamp seconds) {
    if (seconds < 0) {
        seconds = 0;
    }
    const auto hours = seconds / 3600;
    const auto minutes = (seconds % 3600) / 60;
    const auto remainingSeconds = seconds % 60;
    std::ostringstream out;
    out << hours << " ч " << std::setw(2) << std::setfill('0') << minutes
        << " мин " << std::setw(2) << remainingSeconds << " сек";
    return out.str();
}

}
