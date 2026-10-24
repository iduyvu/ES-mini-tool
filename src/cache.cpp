#include "cache.hpp"

Cache::Cache(Database& db) : db(db) {}

void Cache::load_all() {
    cases = db.get_all_cases();
    // Load visits
    for (const auto& c : cases) {
        auto case_visits = db.get_visits_for_case(c.case_id);
        visits.insert(visits.end(), case_visits.begin(), case_visits.end());
    }
    // Load items
    for (const auto& v : visits) {
        auto visit_items = db.get_items_for_visit(v.visit_id);
        items.insert(items.end(), visit_items.begin(), visit_items.end());
    }
}

void Cache::add_case(const Case& c) {
    cases.push_back(c);
    db.insert_case(c);
}

void Cache::add_visit(const SiteVisit& v) {
    visits.push_back(v);
    db.insert_visit(v);
}

void Cache::add_item(const VisitPartItem& item) {
    items.push_back(item);
    db.insert_item(item);
}

bool Cache::update_case(const Case& c) {
    for (auto& cached_c : cases) {
        if (cached_c.case_id == c.case_id) {
            cached_c = c;
            return db.update_case(c);
        }
    }
    return false;
}

void Cache::add_zcl_mapping(const ZclMapping& zcl) {
    zcl_map[zcl.original_part_no] = zcl.zcl_item_code;
    db.insert_zcl_mapping(zcl);
}

Case* Cache::get_case(const std::string& case_id) {
    for (auto& c : cases) {
        if (c.case_id == case_id) return &c;
    }
    return nullptr;
}

std::vector<SiteVisit*> Cache::get_visits_for_case(const std::string& case_id) {
    std::vector<SiteVisit*> result;
    for (auto& v : visits) {
        if (v.case_id == case_id) result.push_back(&v);
    }
    return result;
}

std::vector<VisitPartItem*> Cache::get_items_for_visit(int visit_id) {
    std::vector<VisitPartItem*> result;
    for (auto& i : items) {
        if (i.visit_id == visit_id) result.push_back(&i);
    }
    return result;
}

std::string Cache::resolve_zcl(const std::string& out_part_no) {
    if (zcl_map.find(out_part_no) != zcl_map.end()) {
        return zcl_map[out_part_no];
    }
    return db.get_zcl_mapping(out_part_no);
}
