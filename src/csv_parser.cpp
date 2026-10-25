#include "csv_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

CSVParser::CSVParser(const std::string& filepath) : filepath(filepath) {}

std::vector<std::string> CSVParser::split_csv_line(const std::string& line) {
    std::vector<std::string> result;
    bool in_quotes = false;
    std::string current_field;

    for (char c : line) {
        if (c == '\"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            result.push_back(current_field);
            current_field.clear();
        } else {
            current_field += c;
        }
    }
    result.push_back(current_field);
    return result;
}

bool CSVParser::parse() {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    std::string line;
    if (std::getline(file, line)) {
        // Parse header
        auto header = split_csv_line(line);
        if (header.size() > 2) {
            for (size_t i = 2; i < header.size(); ++i) {
                locations.push_back(header[i]);
            }
        }
    }

    while (std::getline(file, line)) {
        auto fields = split_csv_line(line);
        if (fields.size() >= 2) {
            InventoryItem item;
            item.part_no = fields[0];
            item.description = fields[1];

            for (size_t i = 2; i < fields.size() && i - 2 < locations.size(); ++i) {
                try {
                    item.stock_locations[locations[i - 2]] = std::stoi(fields[i]);
                } catch (...) {
                    item.stock_locations[locations[i - 2]] = 0;
                }
            }
            inventory[item.part_no] = item;
        }
    }
    return true;
}

bool CSVParser::is_valid_part(const std::string& part_no) const {
    return inventory.find(part_no) != inventory.end();
}

std::vector<std::string> CSVParser::search_zcl_parts(const std::string& query) const {
    std::vector<std::string> results;
    for (const auto& pair : inventory) {
        if (pair.first.rfind("ZCL-", 0) == 0 && pair.first.find(query) != std::string::npos) {
            results.push_back(pair.first);
        }
    }
    return results;
}
