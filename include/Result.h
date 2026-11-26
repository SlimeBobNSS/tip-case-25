#pragma once
#include <optional>
#include <string>
#include <utility>
namespace timetracker {
template<class T> struct Result {
    std::optional<T> value;
    std::string error;
    explicit operator bool() const { return value.has_value(); }
    static Result success(T item) { return {std::move(item), {}}; }
    static Result failure(std::string message) { return {std::nullopt, std::move(message)}; }
};
}
