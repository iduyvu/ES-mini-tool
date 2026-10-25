#include "state_machine.hpp"
#include "utils.hpp"

namespace fsm {

ValidationResult can_close_case(Cache& cache, const std::string& case_id) {
    ValidationResult res{true, {}};

    Case* c = cache.get_case(case_id);
    if (!c) {
        res.is_valid = false;
        res.errors.push_back("Case not found.");
        return res;
    }

    // [Rule 1]
    if (!c->issue_resolved_confirmed) {
        res.is_valid = false;
        res.errors.push_back("Issue not confirmed as resolved.");
    }

    auto visits = cache.get_visits_for_case(case_id);
    for (auto v : visits) {
        // [Rule 2]
        if (!v->sr_collected) {
            res.is_valid = false;
            res.errors.push_back("Missing Service Report for visit seq " + std::to_string(v->visit_seq));
        }

        auto items = cache.get_items_for_visit(v->visit_id);
        if (!items.empty()) {
            // [Rule 3]
            if (!v->huy_posted_tl) {
                res.is_valid = false;
                res.errors.push_back("TL not posted for visit seq " + std::to_string(v->visit_seq));
            }
            if (!v->bbbg_stamped_returned) {
                res.is_valid = false;
                res.errors.push_back("Stamped BBBG not returned for visit seq " + std::to_string(v->visit_seq));
            }

            for (auto item : items) {
                // [Rule 4]
                if (item->part_outcome == "UNUSED_RETURN" && !item->unused_returned_wh) {
                    res.is_valid = false;
                    res.errors.push_back("Unused part " + item->out_part_no + " not returned to WH.");
                }
                if (item->part_outcome == "INSTALLED_CLAIM" && !item->faulty_returned_wh) {
                    res.is_valid = false;
                    res.errors.push_back("Faulty part " + item->out_part_no + " not returned to WH.");
                }

                // [Rule 5]
                if (item->vendor_status == "APPROVED") {
                    if (!item->zcl_transferred) {
                        res.is_valid = false;
                        res.errors.push_back("ZCL not transferred for " + item->out_part_no);
                    }
                    if (!item->faulty_posted_huy || !item->wrr_cc_hoa) {
                        res.is_valid = false;
                        res.errors.push_back("Faulty not posted or WRR not started for " + item->out_part_no);
                    }
                    if (item->is_refurbished && (!item->clcus_worksheet_done || !item->tl_new_part_pulled_back)) {
                        res.is_valid = false;
                        res.errors.push_back("Refurbished logistics not complete for " + item->out_part_no);
                    }
                }
            }
        }
    }

    return res;
}

bool close_case(Cache& cache, const std::string& case_id, const std::string& close_date) {
    auto res = can_close_case(cache, case_id);
    if (!res.is_valid) return false;

    Case* c = cache.get_case(case_id);
    if (c) {
        c->current_state = "CLOSED";
        c->closed_date = utils::parse_and_standardize_date(close_date);

        return cache.update_case(*c);
    }
    return false;
}

} // namespace fsm
