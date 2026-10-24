#pragma once
#include <string>
#include <vector>
#include "cache.hpp"

namespace fsm {

struct ValidationResult {
    bool is_valid;
    std::vector<std::string> errors;
};

// Pure functions for state transitions and gatekeeper logic
ValidationResult can_close_case(Cache& cache, const std::string& case_id);
bool close_case(Cache& cache, const std::string& case_id, const std::string& close_date);

} // namespace fsm
