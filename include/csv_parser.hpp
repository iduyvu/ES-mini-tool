#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct InventoryItem {
    std::string part_no;
    std::string description;
    std::unordered_map<std::string, int> stock_locations;
};

class CSVParser {
public:
    CSVParser(const std::string& filepath);
    bool parse();

    const std::vector<std::string>& get_locations() const { return locations; }
    const std::unordered_map<std::string, InventoryItem>& get_inventory() const { return inventory; }

    bool is_valid_part(const std::string& part_no) const;
    std::vector<std::string> search_zcl_parts(const std::string& query) const;

private:
    std::string filepath;
    std::vector<std::string> locations;
    std::unordered_map<std::string, InventoryItem> inventory;

    std::vector<std::string> split_csv_line(const std::string& line);
};
