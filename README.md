# ES-mini-tool
- **Dự án:** Hệ thống Quản lý Dịch vụ Kỹ thuật & Điều phối Bảo hành Linh kiện (Service Case & RMA Tracking Engine)
- **Phiên bản:** 1.0 (Production-Ready Prototype)
- **Ngày phát triển:** 24/10/2026

## Build Instructions (Cross-Compile for Windows)
1. Cài đặt CMake, MinGW-w64 trên Linux.
2. `mkdir build && cd build`
3. `cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw.cmake ..`
4. `make`
5. File chạy sẽ được tạo tại `bin/rma_tracker.exe`.

## Dependencies
- FTXUI (Tải tự động qua CMake FetchContent)
- SQLite3 (Tải tự động qua CMake FetchContent)
