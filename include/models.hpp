#pragma once
#include <string>

// DOD POD structs for the 3-Tier Hierarchy

struct Case {
    std::string case_id;
    std::string branch;
    std::string hall;
    std::string site;
    std::string service_item;
    std::string engineer_name;
    std::string initial_symptom;
    std::string initial_action;

    int need_part;
    int has_warranty;
    std::string current_state;

    std::string incident_date;
    std::string closed_date;
    int issue_resolved_confirmed;
    std::string sys_created_at;
};

struct SiteVisit {
    int visit_id;
    std::string case_id;
    int visit_seq;
    std::string visit_type;

    std::string onsite_date;
    std::string engineer_note;

    std::string nav_tl_created_date;
    int huy_posted_tl;
    std::string huy_posted_tl_date;

    std::string bbbg_issued_date;
    int bbbg_stamped_returned;
    std::string bbbg_returned_date;

    int sr_collected;
    std::string sr_collected_date;
};

struct VisitPartItem {
    int item_id;
    int visit_id;
    std::string case_id;

    std::string out_part_no;
    std::string out_serial_no;
    std::string part_outcome;

    int unused_returned_wh;
    std::string unused_wh_date;

    std::string in_faulty_sn;
    int faulty_returned_wh;
    std::string faulty_wh_date;

    std::string claim_raised_date;
    std::string vendor_status;
    std::string vendor_decision_date;

    int is_refurbished;
    std::string zcl_item_code;
    int zcl_transferred;
    std::string zcl_transfer_date;

    int faulty_posted_huy;
    int wrr_cc_hoa;
    std::string wrr_mail_date;

    int clcus_worksheet_done;
    int tl_new_part_pulled_back;
    std::string clcus_date;
};

struct PartLedger {
    int ledger_id;
    std::string case_id;
    int visit_id;
    std::string site;
    std::string engineer_name;
    std::string part_no;
    std::string serial_no;
    std::string zcl_code;
    std::string usage_role;
    int is_refurbished;
    std::string transfer_out_date;
    std::string transfer_in_date;
    std::string current_location;
    int is_settled;
};

struct ZclMapping {
    std::string original_part_no;
    std::string zcl_item_code;
    std::string updated_at;
};
