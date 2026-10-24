#include "db.hpp"
#include <iostream>

Database::Database(const std::string& db_path) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
    }
}

bool Database::execute_sql(const std::string& sql) {
    if (!db) return false;
    char* errmsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool Database::init_schema() {
    const std::string sql = R"(
        CREATE TABLE IF NOT EXISTS cases (
            case_id TEXT PRIMARY KEY,
            branch TEXT NOT NULL,
            hall TEXT,
            site TEXT NOT NULL,
            service_item TEXT NOT NULL,
            engineer_name TEXT NOT NULL,
            initial_symptom TEXT,
            initial_action TEXT,
            need_part INTEGER DEFAULT 0,
            has_warranty INTEGER DEFAULT 0,
            current_state TEXT NOT NULL,
            incident_date TEXT NOT NULL,
            closed_date TEXT,
            issue_resolved_confirmed INTEGER DEFAULT 0,
            sys_created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS site_visits (
            visit_id INTEGER PRIMARY KEY AUTOINCREMENT,
            case_id TEXT NOT NULL,
            visit_seq INTEGER NOT NULL,
            visit_type TEXT NOT NULL,
            onsite_date TEXT NOT NULL,
            engineer_note TEXT,
            nav_tl_created_date TEXT,
            huy_posted_tl INTEGER DEFAULT 0,
            huy_posted_tl_date TEXT,
            bbbg_issued_date TEXT,
            bbbg_stamped_returned INTEGER DEFAULT 0,
            bbbg_returned_date TEXT,
            sr_collected INTEGER DEFAULT 0,
            sr_collected_date TEXT,
            FOREIGN KEY(case_id) REFERENCES cases(case_id)
        );

        CREATE TABLE IF NOT EXISTS visit_part_items (
            item_id INTEGER PRIMARY KEY AUTOINCREMENT,
            visit_id INTEGER NOT NULL,
            case_id TEXT NOT NULL,
            out_part_no TEXT NOT NULL,
            out_serial_no TEXT NOT NULL,
            part_outcome TEXT NOT NULL,
            unused_returned_wh INTEGER DEFAULT 0,
            unused_wh_date TEXT,
            in_faulty_sn TEXT,
            faulty_returned_wh INTEGER DEFAULT 0,
            faulty_wh_date TEXT,
            claim_raised_date TEXT,
            vendor_status TEXT DEFAULT 'NONE',
            vendor_decision_date TEXT,
            is_refurbished INTEGER DEFAULT 0,
            zcl_item_code TEXT,
            zcl_transferred INTEGER DEFAULT 0,
            zcl_transfer_date TEXT,
            faulty_posted_huy INTEGER DEFAULT 0,
            wrr_cc_hoa INTEGER DEFAULT 0,
            wrr_mail_date TEXT,
            clcus_worksheet_done INTEGER DEFAULT 0,
            tl_new_part_pulled_back INTEGER DEFAULT 0,
            clcus_date TEXT,
            FOREIGN KEY(visit_id) REFERENCES site_visits(visit_id)
        );

        CREATE TABLE IF NOT EXISTS part_ledger (
            ledger_id INTEGER PRIMARY KEY AUTOINCREMENT,
            case_id TEXT NOT NULL,
            visit_id INTEGER NOT NULL,
            site TEXT NOT NULL,
            engineer_name TEXT NOT NULL,
            part_no TEXT NOT NULL,
            serial_no TEXT NOT NULL,
            zcl_code TEXT,
            usage_role TEXT NOT NULL,
            is_refurbished INTEGER DEFAULT 0,
            transfer_out_date TEXT,
            transfer_in_date TEXT,
            current_location TEXT NOT NULL,
            is_settled INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS zcl_mappings (
            original_part_no TEXT PRIMARY KEY,
            zcl_item_code TEXT NOT NULL,
            updated_at TEXT
        );

        CREATE INDEX IF NOT EXISTS idx_visits_case ON site_visits(case_id);
        CREATE INDEX IF NOT EXISTS idx_items_visit ON visit_part_items(visit_id);
        CREATE INDEX IF NOT EXISTS idx_ledger_part ON part_ledger(part_no, serial_no);
        CREATE INDEX IF NOT EXISTS idx_ledger_settled ON part_ledger(is_settled);
    )";
    return execute_sql(sql);
}

std::vector<Case> Database::get_all_cases() {
    std::vector<Case> cases;
    if (!db) return cases;

    const char* sql = "SELECT case_id, branch, hall, site, service_item, engineer_name, "
                      "initial_symptom, initial_action, need_part, has_warranty, current_state, "
                      "incident_date, closed_date, issue_resolved_confirmed, sys_created_at "
                      "FROM cases";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Case c;
            c.case_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            c.branch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* hall_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            c.hall = hall_ptr ? hall_ptr : "";
            c.site = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            c.service_item = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            c.engineer_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            const char* initial_symptom_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            c.initial_symptom = initial_symptom_ptr ? initial_symptom_ptr : "";
            const char* initial_action_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            c.initial_action = initial_action_ptr ? initial_action_ptr : "";
            c.need_part = sqlite3_column_int(stmt, 8);
            c.has_warranty = sqlite3_column_int(stmt, 9);
            c.current_state = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
            c.incident_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            const char* closed_date_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            c.closed_date = closed_date_ptr ? closed_date_ptr : "";
            c.issue_resolved_confirmed = sqlite3_column_int(stmt, 13);
            const char* sys_created_at_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
            c.sys_created_at = sys_created_at_ptr ? sys_created_at_ptr : "";

            cases.push_back(c);
        }
        sqlite3_finalize(stmt);
    }
    return cases;
}

bool Database::insert_case(const Case& c) {
    if (!db) return false;
    const char* sql = "INSERT INTO cases (case_id, branch, hall, site, service_item, engineer_name, "
                      "initial_symptom, initial_action, need_part, has_warranty, current_state, "
                      "incident_date, closed_date, issue_resolved_confirmed, sys_created_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, c.case_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c.branch.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c.hall.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c.site.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, c.service_item.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, c.engineer_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, c.initial_symptom.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, c.initial_action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, c.need_part);
    sqlite3_bind_int(stmt, 10, c.has_warranty);
    sqlite3_bind_text(stmt, 11, c.current_state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, c.incident_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, c.closed_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, c.issue_resolved_confirmed);
    sqlite3_bind_text(stmt, 15, c.sys_created_at.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::update_case(const Case& c) {
    if (!db) return false;
    const char* sql = "UPDATE cases SET branch=?, hall=?, site=?, service_item=?, engineer_name=?, "
                      "initial_symptom=?, initial_action=?, need_part=?, has_warranty=?, current_state=?, "
                      "incident_date=?, closed_date=?, issue_resolved_confirmed=?, sys_created_at=? "
                      "WHERE case_id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, c.branch.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c.hall.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c.site.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c.service_item.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, c.engineer_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, c.initial_symptom.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, c.initial_action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, c.need_part);
    sqlite3_bind_int(stmt, 9, c.has_warranty);
    sqlite3_bind_text(stmt, 10, c.current_state.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, c.incident_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, c.closed_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, c.issue_resolved_confirmed);
    sqlite3_bind_text(stmt, 14, c.sys_created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, c.case_id.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<SiteVisit> Database::get_visits_for_case(const std::string& case_id) {
    std::vector<SiteVisit> visits;
    if (!db) return visits;

    const char* sql = "SELECT visit_id, case_id, visit_seq, visit_type, onsite_date, engineer_note, "
                      "nav_tl_created_date, huy_posted_tl, huy_posted_tl_date, bbbg_issued_date, "
                      "bbbg_stamped_returned, bbbg_returned_date, sr_collected, sr_collected_date "
                      "FROM site_visits WHERE case_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, case_id.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SiteVisit v;
            v.visit_id = sqlite3_column_int(stmt, 0);
            v.case_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            v.visit_seq = sqlite3_column_int(stmt, 2);
            v.visit_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            v.onsite_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            const char* note_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            v.engineer_note = note_ptr ? note_ptr : "";
            const char* nav_tl_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            v.nav_tl_created_date = nav_tl_ptr ? nav_tl_ptr : "";
            v.huy_posted_tl = sqlite3_column_int(stmt, 7);
            const char* huy_date_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            v.huy_posted_tl_date = huy_date_ptr ? huy_date_ptr : "";
            const char* bbbg_issued_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
            v.bbbg_issued_date = bbbg_issued_ptr ? bbbg_issued_ptr : "";
            v.bbbg_stamped_returned = sqlite3_column_int(stmt, 10);
            const char* bbbg_ret_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            v.bbbg_returned_date = bbbg_ret_ptr ? bbbg_ret_ptr : "";
            v.sr_collected = sqlite3_column_int(stmt, 12);
            const char* sr_date_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
            v.sr_collected_date = sr_date_ptr ? sr_date_ptr : "";
            visits.push_back(v);
        }
        sqlite3_finalize(stmt);
    }
    return visits;
}

bool Database::insert_visit(const SiteVisit& v) {
    if (!db) return false;
    const char* sql = "INSERT INTO site_visits (case_id, visit_seq, visit_type, onsite_date, engineer_note, "
                      "nav_tl_created_date, huy_posted_tl, huy_posted_tl_date, bbbg_issued_date, "
                      "bbbg_stamped_returned, bbbg_returned_date, sr_collected, sr_collected_date) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, v.case_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, v.visit_seq);
    sqlite3_bind_text(stmt, 3, v.visit_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, v.onsite_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, v.engineer_note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, v.nav_tl_created_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, v.huy_posted_tl);
    sqlite3_bind_text(stmt, 8, v.huy_posted_tl_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, v.bbbg_issued_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, v.bbbg_stamped_returned);
    sqlite3_bind_text(stmt, 11, v.bbbg_returned_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, v.sr_collected);
    sqlite3_bind_text(stmt, 13, v.sr_collected_date.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::update_visit(const SiteVisit& v) {
    if (!db) return false;
    const char* sql = "UPDATE site_visits SET case_id=?, visit_seq=?, visit_type=?, onsite_date=?, engineer_note=?, "
                      "nav_tl_created_date=?, huy_posted_tl=?, huy_posted_tl_date=?, bbbg_issued_date=?, "
                      "bbbg_stamped_returned=?, bbbg_returned_date=?, sr_collected=?, sr_collected_date=? "
                      "WHERE visit_id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, v.case_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, v.visit_seq);
    sqlite3_bind_text(stmt, 3, v.visit_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, v.onsite_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, v.engineer_note.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, v.nav_tl_created_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, v.huy_posted_tl);
    sqlite3_bind_text(stmt, 8, v.huy_posted_tl_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, v.bbbg_issued_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 10, v.bbbg_stamped_returned);
    sqlite3_bind_text(stmt, 11, v.bbbg_returned_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, v.sr_collected);
    sqlite3_bind_text(stmt, 13, v.sr_collected_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, v.visit_id);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::vector<VisitPartItem> Database::get_items_for_visit(int visit_id) {
    std::vector<VisitPartItem> items;
    if (!db) return items;

    const char* sql = "SELECT item_id, visit_id, case_id, out_part_no, out_serial_no, part_outcome, "
                      "unused_returned_wh, unused_wh_date, in_faulty_sn, faulty_returned_wh, faulty_wh_date, "
                      "claim_raised_date, vendor_status, vendor_decision_date, is_refurbished, zcl_item_code, "
                      "zcl_transferred, zcl_transfer_date, faulty_posted_huy, wrr_cc_hoa, wrr_mail_date, "
                      "clcus_worksheet_done, tl_new_part_pulled_back, clcus_date "
                      "FROM visit_part_items WHERE visit_id = ?";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, visit_id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            VisitPartItem item;
            item.item_id = sqlite3_column_int(stmt, 0);
            item.visit_id = sqlite3_column_int(stmt, 1);
            item.case_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            item.out_part_no = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            item.out_serial_no = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            item.part_outcome = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            item.unused_returned_wh = sqlite3_column_int(stmt, 6);
            const char* u_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            item.unused_wh_date = u_date ? u_date : "";
            const char* in_faulty = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
            item.in_faulty_sn = in_faulty ? in_faulty : "";
            item.faulty_returned_wh = sqlite3_column_int(stmt, 9);
            const char* f_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
            item.faulty_wh_date = f_date ? f_date : "";
            const char* claim_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
            item.claim_raised_date = claim_date ? claim_date : "";
            item.vendor_status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            const char* v_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
            item.vendor_decision_date = v_date ? v_date : "";
            item.is_refurbished = sqlite3_column_int(stmt, 14);
            const char* zcl_code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
            item.zcl_item_code = zcl_code ? zcl_code : "";
            item.zcl_transferred = sqlite3_column_int(stmt, 16);
            const char* zcl_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
            item.zcl_transfer_date = zcl_date ? zcl_date : "";
            item.faulty_posted_huy = sqlite3_column_int(stmt, 18);
            item.wrr_cc_hoa = sqlite3_column_int(stmt, 19);
            const char* wrr_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 20));
            item.wrr_mail_date = wrr_date ? wrr_date : "";
            item.clcus_worksheet_done = sqlite3_column_int(stmt, 21);
            item.tl_new_part_pulled_back = sqlite3_column_int(stmt, 22);
            const char* clcus_date_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 23));
            item.clcus_date = clcus_date_ptr ? clcus_date_ptr : "";

            items.push_back(item);
        }
        sqlite3_finalize(stmt);
    }
    return items;
}

bool Database::insert_item(const VisitPartItem& item) {
    if (!db) return false;
    const char* sql = "INSERT INTO visit_part_items (visit_id, case_id, out_part_no, out_serial_no, part_outcome, "
                      "unused_returned_wh, unused_wh_date, in_faulty_sn, faulty_returned_wh, faulty_wh_date, "
                      "claim_raised_date, vendor_status, vendor_decision_date, is_refurbished, zcl_item_code, "
                      "zcl_transferred, zcl_transfer_date, faulty_posted_huy, wrr_cc_hoa, wrr_mail_date, "
                      "clcus_worksheet_done, tl_new_part_pulled_back, clcus_date) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, item.visit_id);
    sqlite3_bind_text(stmt, 2, item.case_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.out_part_no.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, item.out_serial_no.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, item.part_outcome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, item.unused_returned_wh);
    sqlite3_bind_text(stmt, 7, item.unused_wh_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, item.in_faulty_sn.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, item.faulty_returned_wh);
    sqlite3_bind_text(stmt, 10, item.faulty_wh_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, item.claim_raised_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, item.vendor_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, item.vendor_decision_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, item.is_refurbished);
    sqlite3_bind_text(stmt, 15, item.zcl_item_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, item.zcl_transferred);
    sqlite3_bind_text(stmt, 17, item.zcl_transfer_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 18, item.faulty_posted_huy);
    sqlite3_bind_int(stmt, 19, item.wrr_cc_hoa);
    sqlite3_bind_text(stmt, 20, item.wrr_mail_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 21, item.clcus_worksheet_done);
    sqlite3_bind_int(stmt, 22, item.tl_new_part_pulled_back);
    sqlite3_bind_text(stmt, 23, item.clcus_date.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

bool Database::update_item(const VisitPartItem& item) {
    if (!db) return false;
    const char* sql = "UPDATE visit_part_items SET visit_id=?, case_id=?, out_part_no=?, out_serial_no=?, part_outcome=?, "
                      "unused_returned_wh=?, unused_wh_date=?, in_faulty_sn=?, faulty_returned_wh=?, faulty_wh_date=?, "
                      "claim_raised_date=?, vendor_status=?, vendor_decision_date=?, is_refurbished=?, zcl_item_code=?, "
                      "zcl_transferred=?, zcl_transfer_date=?, faulty_posted_huy=?, wrr_cc_hoa=?, wrr_mail_date=?, "
                      "clcus_worksheet_done=?, tl_new_part_pulled_back=?, clcus_date=? "
                      "WHERE item_id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, item.visit_id);
    sqlite3_bind_text(stmt, 2, item.case_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.out_part_no.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, item.out_serial_no.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, item.part_outcome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, item.unused_returned_wh);
    sqlite3_bind_text(stmt, 7, item.unused_wh_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, item.in_faulty_sn.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, item.faulty_returned_wh);
    sqlite3_bind_text(stmt, 10, item.faulty_wh_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, item.claim_raised_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, item.vendor_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, item.vendor_decision_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 14, item.is_refurbished);
    sqlite3_bind_text(stmt, 15, item.zcl_item_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, item.zcl_transferred);
    sqlite3_bind_text(stmt, 17, item.zcl_transfer_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 18, item.faulty_posted_huy);
    sqlite3_bind_int(stmt, 19, item.wrr_cc_hoa);
    sqlite3_bind_text(stmt, 20, item.wrr_mail_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 21, item.clcus_worksheet_done);
    sqlite3_bind_int(stmt, 22, item.tl_new_part_pulled_back);
    sqlite3_bind_text(stmt, 23, item.clcus_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 24, item.item_id);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}

std::string Database::get_zcl_mapping(const std::string& original_part_no) {
    if (!db) return "";
    std::string result = "";
    const char* sql = "SELECT zcl_item_code FROM zcl_mappings WHERE original_part_no = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, original_part_no.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* code = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (code) result = code;
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

bool Database::insert_zcl_mapping(const ZclMapping& mapping) {
    if (!db) return false;
    const char* sql = "INSERT OR REPLACE INTO zcl_mappings (original_part_no, zcl_item_code, updated_at) "
                      "VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, mapping.original_part_no.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, mapping.zcl_item_code.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, mapping.updated_at.c_str(), -1, SQLITE_TRANSIENT);

    bool success = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return success;
}
