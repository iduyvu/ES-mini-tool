#include "utils.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#endif

namespace utils {

std::string get_current_date() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
    return ss.str();
}

std::string get_date_offset(int days) {
    auto now = std::chrono::system_clock::now();
    auto offset_time = now + std::chrono::hours(24 * days);
    auto in_time_t = std::chrono::system_clock::to_time_t(offset_time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
    return ss.str();
}

std::string parse_and_standardize_date(const std::string& input) {
    if (input.empty()) {
        return get_current_date();
    }

    std::regex negative_number("^-\\d+$");
    if (std::regex_match(input, negative_number)) {
        int days = std::stoi(input);
        return get_date_offset(days);
    }

    std::regex short_date("^(\\d{1,2})[/\\-](\\d{1,2})$");
    std::smatch match;
    if (std::regex_match(input, match, short_date)) {
        int day = std::stoi(match[1].str());
        int month = std::stoi(match[2].str());

        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&in_time_t);
        int current_year = local_time->tm_year + 1900;

        std::stringstream ss;
        ss << current_year << "-"
           << std::setw(2) << std::setfill('0') << month << "-"
           << std::setw(2) << std::setfill('0') << day;
        return ss.str();
    }

    return input; // Fallback: return as-is
}

bool copy_to_clipboard(const std::string& text) {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();

    HGLOBAL hGlob = GlobalAlloc(GMEM_FIXED, text.size() + 1);
    if (!hGlob) {
        CloseClipboard();
        return false;
    }

    memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
    GlobalUnlock(hGlob);
    SetClipboardData(CF_TEXT, hGlob);
    CloseClipboard();
    return true;
#else
    // Stub for Linux environment where we are building but not executing clipboard code natively
    std::cout << "[Mock Clipboard Copy]:\n" << text << "\n";
    return true;
#endif
}

} // namespace utils
