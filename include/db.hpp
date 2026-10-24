#pragma once
#include <string>
#include <vector>
#include "models.hpp"
#include "sqlite3.h"

class Database {
public:
    Database(const std::string& db_path);
    ~Database();

    bool init_schema();

    // CRUD Operations for Cases
    std::vector<Case> get_all_cases();
    bool insert_case(const Case& c);
    bool update_case(const Case& c);

    // CRUD Operations for SiteVisits
    std::vector<SiteVisit> get_visits_for_case(const std::string& case_id);
    bool insert_visit(const SiteVisit& v);
    bool update_visit(const SiteVisit& v);

    // CRUD Operations for VisitPartItems
    std::vector<VisitPartItem> get_items_for_visit(int visit_id);
    bool insert_item(const VisitPartItem& item);
    bool update_item(const VisitPartItem& item);

    // CRUD Operations for ZclMappings
    std::string get_zcl_mapping(const std::string& original_part_no);
    bool insert_zcl_mapping(const ZclMapping& mapping);

private:
    sqlite3* db;
    bool execute_sql(const std::string& sql);
};
