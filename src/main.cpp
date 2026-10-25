#include <iostream>
#include "db.hpp"
#include "cache.hpp"
#include "csv_parser.hpp"
#include "ui.hpp"

int main() {
    Database db("rma_tracker.db");
    if (!db.init_schema()) {
        std::cerr << "Failed to initialize database schema." << std::endl;
        return 1;
    }

    CSVParser parser("nav_inventory.csv");
    parser.parse();

    Cache cache(db);
    cache.load_all();

    ui::render_dashboard(cache);

    return 0;
}
