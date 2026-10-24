cmake_minimum_required(VERSION 3.14)
project(rma_tracker C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0135 NEW)

include(FetchContent)

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
  GIT_TAG v5.0.0
)
FetchContent_MakeAvailable(ftxui)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

# Fetch SQLite
include(FetchContent)
FetchContent_Declare(
  sqlite3
  URL https://www.sqlite.org/2023/sqlite-amalgamation-3430100.zip
)
FetchContent_MakeAvailable(sqlite3)

# Create a library for sqlite3
add_library(sqlite3 ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)

add_executable(rma_tracker
    src/main.cpp
    src/utils.cpp
    src/db.cpp
    src/csv_parser.cpp
    src/cache.cpp
    src/state_machine.cpp
    src/ui.cpp
)

target_include_directories(rma_tracker PRIVATE include)
target_link_libraries(rma_tracker PRIVATE sqlite3)

# Link FTXUI, sqlite3, and necessary Windows libraries
target_link_libraries(rma_tracker
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

if (WIN32)
    target_link_libraries(rma_tracker PRIVATE ws2_32)
endif()
