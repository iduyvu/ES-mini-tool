#pragma once
#include <string>

namespace utils {
    std::string parse_and_standardize_date(const std::string& input);
    bool copy_to_clipboard(const std::string& text);
    std::string get_current_date();
}
