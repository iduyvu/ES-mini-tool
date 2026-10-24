#pragma once
#include <vector>
#include <unordered_map>
#include "models.hpp"
#include "db.hpp"

// Data-Oriented Cache for fast operations
class Cache {
public:
    Cache(Database& db);

    void load_all();

    std::vector<Case>& get_cases() { return cases; }
    std::vector<SiteVisit>& get_visits() { return visits; }
    std::vector<VisitPartItem>& get_items() { return items; }
    std::vector<PartLedger>& get_ledger() { return ledger; }

    void add_case(const Case& c);
    void add_visit(const SiteVisit& v);
    void add_item(const VisitPartItem& item);
    void add_zcl_mapping(const ZclMapping& zcl);

    bool update_case(const Case& c);

    Case* get_case(const std::string& case_id);
    std::vector<SiteVisit*> get_visits_for_case(const std::string& case_id);
    std::vector<VisitPartItem*> get_items_for_visit(int visit_id);

    std::string resolve_zcl(const std::string& out_part_no);

private:
    Database& db;
    std::vector<Case> cases;
    std::vector<SiteVisit> visits;
    std::vector<VisitPartItem> items;
    std::vector<PartLedger> ledger;
    std::unordered_map<std::string, std::string> zcl_map;
};
