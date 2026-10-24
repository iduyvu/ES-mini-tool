#include "ui.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include "utils.hpp"
#include "state_machine.hpp"
#include <iostream>

using namespace ftxui;

namespace ui {

std::string create_post_tl_email(const std::string& site, const std::string& branch, const std::string& hall, const std::string& case_id, const std::string& parts) {
    return "Chào Huy, nhờ Huy post giúp lệnh TL trên NAV cho Site: " + site +
           " (" + branch + " - " + hall + ") nhé. Mã Case: " + case_id + " | Danh sách Part: " + parts + ". Cảm ơn Huy!";
}

void render_dashboard(Cache& cache) {
    auto screen = ScreenInteractive::TerminalOutput();

    int tab_index = 0;
    std::vector<std::string> tab_entries = {
        "1. Active Cases",
        "2. Create New Case",
        "3. Site Visits & Parts",
        "4. Email Templates",
        "5. Exit"
    };
    auto tab_selection = Menu(&tab_entries, &tab_index);

    // -- Tab 1: Active Cases --
    std::string close_case_id;
    auto input_close_id = Input(&close_case_id, "Enter Case ID to close");
    std::string close_status;
    auto btn_close = Button("Close Case", [&] {
        auto res = fsm::can_close_case(cache, close_case_id);
        if (res.is_valid) {
            if (fsm::close_case(cache, close_case_id, "")) {
                close_status = "Case " + close_case_id + " closed successfully.";
            } else {
                close_status = "Failed to update database.";
            }
        } else {
            close_status = "Cannot close. Errors: ";
            for (const auto& err : res.errors) close_status += err + " | ";
        }
    });

    auto render_cases = [&]() {
        Elements case_elements;
        auto cases = cache.get_cases();
        if (cases.empty()) {
            case_elements.push_back(text("No cases found.") | dim);
        } else {
            for (const auto& c : cases) {
                case_elements.push_back(
                    hbox({
                        text(c.case_id) | bold | color(Color::Cyan),
                        text(" | "),
                        text(c.site) | color(Color::Yellow),
                        text(" | Status: "),
                        text(c.current_state) | (c.current_state == "OPEN" ? color(Color::Green) : color(Color::Red)),
                        text(c.issue_resolved_confirmed ? " | [Resolved]" : " | [Unresolved]")
                    })
                );
            }
        }
        return vbox({
            vbox(std::move(case_elements)),
            separator(),
            hbox(text("Close Case: "), input_close_id->Render(), btn_close->Render()),
            text(close_status) | color(Color::Red)
        });
    };

    auto tab_active_cases = Renderer(Container::Vertical({input_close_id, btn_close}), [&] { return render_cases(); });

    // -- Tab 2: Create New Case --
    std::string in_case_id, in_site, in_branch, in_eng, in_item, in_symptom;
    auto input_case_id = Input(&in_case_id, "SR-YYYYMMDD-XX");
    auto input_site = Input(&in_site, "Site Name");
    auto input_branch = Input(&in_branch, "Branch");
    auto input_eng = Input(&in_eng, "Engineer Name");
    auto input_item = Input(&in_item, "Service Item");
    auto input_symp = Input(&in_symptom, "Initial Symptom");

    auto btn_create = Button("Save Case", [&] {
        if (!in_case_id.empty() && !in_site.empty()) {
            Case c;
            c.case_id = in_case_id;
            c.site = in_site;
            c.branch = in_branch;
            c.engineer_name = in_eng;
            c.service_item = in_item;
            c.initial_symptom = in_symptom;
            c.need_part = 0;
            c.has_warranty = 0;
            c.issue_resolved_confirmed = 0;
            c.current_state = "OPEN";
            c.sys_created_at = utils::get_current_date();
            c.incident_date = utils::get_current_date();
            cache.add_case(c);

            in_case_id = ""; in_site = ""; in_branch = ""; in_eng = ""; in_item = ""; in_symptom = "";
            tab_index = 0;
        }
    });

    auto form_create = Container::Vertical({
        input_case_id,
        input_site,
        input_branch,
        input_eng,
        input_item,
        input_symp,
        btn_create
    });

    auto tab_create = Renderer(form_create, [&] {
        return vbox({
            text("--- Create New Case ---") | bold,
            separator(),
            hbox(text("Case ID: "), input_case_id->Render()),
            hbox(text("Site: "), input_site->Render()),
            hbox(text("Branch: "), input_branch->Render()),
            hbox(text("Engineer: "), input_eng->Render()),
            hbox(text("Service Item: "), input_item->Render()),
            hbox(text("Symptom: "), input_symp->Render()),
            separator(),
            btn_create->Render()
        }) | border;
    });

    // -- Tab 3: Visits & Parts Mock (Partial Implementation for completeness of architecture)
    std::string visit_case_id, visit_type;
    auto input_v_case = Input(&visit_case_id, "Case ID");
    auto input_v_type = Input(&visit_type, "CALIBRATE_ONLY / TEMP_TL_DISPATCH");
    auto btn_add_visit = Button("Add Visit", [&] {
        if (!visit_case_id.empty()) {
            SiteVisit v;
            v.case_id = visit_case_id;
            v.visit_seq = cache.get_visits_for_case(visit_case_id).size() + 1;
            v.visit_type = visit_type;
            v.onsite_date = utils::get_current_date();
            v.sr_collected = 0;
            v.huy_posted_tl = 0;
            v.bbbg_stamped_returned = 0;
            cache.add_visit(v);
            visit_type = "";
        }
    });

    auto form_visits = Container::Vertical({input_v_case, input_v_type, btn_add_visit});
    auto tab_visits = Renderer(form_visits, [&] {
        return vbox({
            text("--- Add Site Visit ---") | bold,
            hbox(text("Case ID: "), input_v_case->Render()),
            hbox(text("Visit Type: "), input_v_type->Render()),
            btn_add_visit->Render()
        });
    });

    // -- Tab 4: Email Templates --
    std::string email_status = "";
    auto btn_mail_tl = Button("Copy 'Post TL' Email", [&] {
        std::string mail = create_post_tl_email("TestSite", "HCM", "Hall A", "SR-20261024-01", "PART-123 (SN: 999)");
        if (utils::copy_to_clipboard(mail)) {
            email_status = "Copied to clipboard!";
        } else {
            email_status = "Failed to copy.";
        }
    });

    auto tab_emails_container = Container::Vertical({ btn_mail_tl });
    auto tab_emails = Renderer(tab_emails_container, [&] {
        return vbox({
            text("--- Email Templates ---") | bold,
            separator(),
            btn_mail_tl->Render(),
            text(email_status) | color(Color::Green)
        });
    });

    // -- Tab 5: Exit --
    auto tab_exit = Renderer([&] {
        screen.Exit();
        return text("Exiting...");
    });

    auto tab_content = Container::Tab({
        tab_active_cases,
        tab_create,
        tab_visits,
        tab_emails,
        tab_exit
    }, &tab_index);

    auto main_container = Container::Horizontal({
        tab_selection,
        tab_content
    });

    auto main_renderer = Renderer(main_container, [&] {
        return vbox({
            text(" RMA Tracking Engine - Production-Ready Prototype v1.0 ") | bold | inverted | center,
            separator(),
            hbox({
                vbox({
                    text("Menu") | bold,
                    separator(),
                    tab_selection->Render()
                }) | border | size(WIDTH, LESS_THAN, 30),
                tab_content->Render() | border | flex
            }) | flex
        });
    });

    screen.Loop(main_renderer);
}

} // namespace ui
