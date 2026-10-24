# TÀI LIỆU ĐẶC TẢ KỸ THUẬT HỆ THỐNG (SYSTEM TECHNICAL SPECIFICATION)

- **Tên dự án:** Hệ thống Quản lý Dịch vụ Kỹ thuật & Điều phối Bảo hành Linh kiện (Service Case & RMA Tracking Engine)
- **Phiên bản:** 1.0 (Production-Ready Prototype)
- **Ngày ban hành:** 24/10/2026
- **Ngôn ngữ & Nền tảng:** Modern C++ (C++17 / C++20) | SQLite3 | Windows CLI/TUI (Zero-Admin Portable)
- **Định hướng Kiến trúc:** Data-Oriented Design (DOD) kết hợp Functional Programming (Pure State Machine)

---

## 1. GIỚI THIỆU & PHẠM VI HỆ THỐNG (INTRODUCTION & SCOPE)

### 1.1. Mục đích Tài liệu
Tài liệu này quy định chi tiết kiến trúc phần mềm, cấu trúc cơ sở dữ liệu, luồng dữ liệu (Dataflow), máy trạng thái (State Machine) và các quy tắc kiểm chứng nghiệp vụ (Validation Rules) cho ứng dụng theo dõi Service Case và RMA nội bộ. Tài liệu đóng vai trò là bản thiết kế gốc (Blueprint) phục vụ việc phát triển, bảo trì và chuyển giao cho nhân sự kế nhiệm hoặc cung cấp ngữ cảnh cho AI Coding Agents.

### 1.2. Mục tiêu Nghiệp vụ
1. **Theo dõi đầu-cuối (End-to-End Tracking):** Kiểm soát 100% vòng đời của một Service Case từ lúc tiếp nhận yêu cầu cho đến khi thu hồi đầy đủ chứng từ, linh kiện và đóng case[cite: 1].
2. **Quản lý Đa Chuyến đi & Đa Linh kiện (Multi-Visit & Multi-Part):** Phản ánh chính xác thực tế kỹ thuật khi một sự cố cần kỹ sư xuống hiện trường nhiều lần, mỗi lần mang theo một hoặc nhiều linh kiện để thử nghiệm (TL) trước khi chốt phương án bảo hành chính thức với hãng[cite: 1].
3. **Đồng bộ & Đối soát ERP (NAV):** Tận dụng dữ liệu tồn kho xuất ra từ hệ thống ERP (file CSV) để chống nhập sai mã vật tư, tự động hóa việc ghi nhớ mã bảo hành (`ZCL-`) và theo dõi sát sao các lệnh chờ hạch toán trên hệ thống NAV[cite: 1].
4. **Độc lập & Gọn nhẹ (Portable & Zero-Admin):** Ứng dụng biên dịch ra file thực thi độc lập (`.exe`), lưu trữ dữ liệu cục bộ trên một file SQLite (`.db`) duy nhất, chạy trực tiếp trên máy tính công ty (Domain-joined) mà không yêu cầu quyền Administrator để cài đặt.

### 1.3. Ma trận Nhân sự & Điểm chạm Nghiệp vụ (Stakeholders)

| Tác nhân (Actor) | Vai trò trong quy trình | Trách nhiệm & Điểm chạm cần hệ thống theo dõi |
| :--- | :--- | :--- |
| **User (Người vận hành)** | Điều phối viên trung tâm | Tiếp nhận thông tin, mở/đóng case, tạo lệnh TL/Transfer/Worksheet trên NAV, soạn BBBG, quản lý kho vật lý và gửi email điều phối[cite: 1]. |
| **Kỹ sư (Engineer)** | Kỹ thuật viên hiện trường | Kiểm tra thiết bị, báo mã Part/Serial/Symptom, đi site cân chỉnh hoặc thay part, gửi email claim hãng, xin mộc khách hàng lên BBBG và nộp Service Report (SR)[cite: 1]. |
| **Huy (NAV Accounting)** | Kiểm soát hạch toán NAV | Nhận email từ User để thực hiện thao tác **Post** phiếu ứng part (TL) cho site và **Post Faulty Item** trên hệ thống NAV[cite: 1]. |
| **Thái (NAV Master Data)** | Quản trị danh mục vật tư | Tạo mới mã item bảo hành trên NAV (luôn bắt đầu bằng tiền tố `ZCL-`) khi linh kiện chưa có mã ZCL tương ứng[cite: 1]. |
| **Hoa (Logistics / WRR)** | Phụ trách thủ tục trả hàng | Nhận thông tin (qua CC email) khi xác linh kiện lỗi được post để thực hiện thủ tục **WRR** (Warranty Return/Repair)[cite: 1]. |
| **Site (Khách hàng)** | Đơn vị sử dụng thiết bị | Ký tên và đóng mộc xác nhận lên Biên bản bàn giao (BBBG) mỗi đợt kỹ sư giao/thay linh kiện[cite: 1]. |
| **Hãng (Vendor)** | Nhà sản xuất thiết bị | Tiếp nhận yêu cầu bảo hành từ Kỹ sư, phản hồi phê duyệt (**Approved**) hoặc từ chối (**Rejected**) và cấp trả linh kiện RMA (Mới hoặc Refurbished)[cite: 1]. |

---

## 2. KIẾN TRÚC TỔNG QUÁT (HIGH-LEVEL SYSTEM ARCHITECTURE)

Hệ thống loại bỏ mô hình Hướng đối tượng phân cấp sâu (Deep OOP Inheritance) và áp dụng kiến trúc **4 Tầng Tách Biệt (4-Tier Decoupled Architecture)** dựa trên **Data-Oriented Design (DOD)** và **Functional Programming (FP)**:

```text
+-----------------------------------------------------------------------------------+
| TẦNG 1: PRESENTATION & UX LAYER (CLI / TUI)                                       |
| - Dashboard 4 Góc nhìn hành động | Bộ nhập ngày thông minh (Smart Date Parser)    |
| - Trình sinh mẫu Email tự động đẩy vào Windows Clipboard (Win32 API)              |
+-----------------------------------------------------------------------------------+
                                         | (Commands / User Inputs)
                                         v
+-----------------------------------------------------------------------------------+
| TẦNG 2: PURE FUNCTIONAL CORE & STATE MACHINE LAYER                                |
| - Hàm thuần túy chuyển trạng thái (State Transitions)                             |
| - Bộ chốt chặn điều kiện đóng Case (Gatekeeper Validation Engine)                 |
+-----------------------------------------------------------------------------------+
                                         | (Plain Old Data Structs / Contiguous Arrays)
                                         v
+-----------------------------------------------------------------------------------+
| TẦNG 3: DATA SYNCHRONIZATION & LEDGER LAYER (IN-MEMORY DOD CACHE)                 |
| - Bộ phân tích CSV Ma trận Kho động (Dynamic Matrix CSV Parser)                   |
| - Từ điển tự học mã ZCL- (Auto-Learning Map) | Sổ cái Linh kiện (Part Ledger)     |
+-----------------------------------------------------------------------------------+
                                         | (SQL Transactions / File Streams)
                                         v
+-----------------------------------------------------------------------------------+
| TẦNG 4: PERSISTENCE LAYER (ZERO-CONFIG STORAGE)                                   |
| - Cơ sở dữ liệu nhúng SQLite3 (rma_tracker.db)                                    |
| - File bản chụp tồn kho xuất từ hệ thống ERP (nav_inventory.csv)                  |
+-----------------------------------------------------------------------------------+
```

### Nguyên tắc Thiết kế Cốt lõi (Design Principles)
1. **Tách biệt Dữ liệu và Hành vi (Data vs. Behavior Separation):** Toàn bộ thực thể nghiệp vụ được biểu diễn bằng các `struct` C++ thuần túy (Plain Old Data - POD), lưu trữ liên tục trên RAM thông qua `std::vector` để tối ưu CPU Cache.
2. **Hàm Thuần Túy (Pure Functions):** Các hàm kiểm tra quy tắc nghiệp vụ và chuyển đổi trạng thái không gây hiệu ứng phụ (No Side-effects), giúp việc kiểm thử (Unit Test) và chuyển đổi sang các ngôn ngữ Functional (như Clojure, Gleam, Rust) trong tương lai diễn ra theo tỷ lệ ánh xạ 1:1.
3. **Mô hình Thời gian Kép (Bitemporal Modeling):** Tách biệt hoàn toàn thời điểm sự việc thực tế diễn ra tại hiện trường (`actual_event_date`) và thời điểm người dùng gõ dữ liệu vào phần mềm (`sys_logged_at`).

---

## 3. THIẾT KẾ CƠ SỞ DỮ LIỆU & CẤU TRÚC DỮ LIỆU (DATABASE & DATA STRUCTURES)

### 3.1. Mô hình Phân cấp 3 Tầng (Case -> SiteVisit -> VisitPartItem)
Để giải quyết bài toán kỹ sư đi hiện trường nhiều lần và mỗi lần mang theo nhiều linh kiện khác nhau:
- **Tầng 1 (`cases`):** Quản lý thông tin định danh của sự cố và trạng thái tổng thể[cite: 1].
- **Tầng 2 (`site_visits`):** Quản lý từng **Chuyến tác nghiệp hiện trường** (Ngày đi site, lệnh TL chung, tờ BBBG chung và tờ Service Report của chuyến đi đó)[cite: 1].
- **Tầng 3 (`visit_part_items`):** Quản lý **Từng linh kiện cụ thể** nằm trong giỏ hàng của chuyến đi đó (Kết quả lắp thử, thu hồi xác hư, trạng thái Claim hãng và hậu cần kho NAV ZCL/CLCUS/WRR của riêng linh kiện đó)[cite: 1].

### 3.2. Đặc tả Schema SQLite (`rma_tracker.db`)

```sql
-- 1. BẢNG HỒ SƠ CHUNG CỦA SERVICE CASE (TẦNG 1)
CREATE TABLE IF NOT EXISTS cases (
    case_id TEXT PRIMARY KEY,              -- Mã Case (VD: SR-20261024-01)
    branch TEXT NOT NULL,                  -- Chi nhánh
    hall TEXT,                             -- Sảnh / Khu vực
    site TEXT NOT NULL,                    -- Tên Site khách hàng
    service_item TEXT NOT NULL,            -- Mã thiết bị (Service Item)
    engineer_name TEXT NOT NULL,           -- Kỹ sư phụ trách
    initial_symptom TEXT,                  -- Triệu chứng lỗi ban đầu
    initial_action TEXT,                   -- Hướng xử lý ban đầu
    
    need_part INTEGER DEFAULT 0,           -- 0: Không thay part, 1: Cần thay part
    has_warranty INTEGER DEFAULT 0,        -- 0: Hết bảo hành (OOW), 1: Còn bảo hành
    current_state TEXT NOT NULL,           -- Trạng thái hiện tại của Case
    
    incident_date TEXT NOT NULL,           -- Ngày thực tế phát sinh/tiếp nhận (YYYY-MM-DD)
    closed_date TEXT,                      -- Ngày thực tế đóng Case (YYYY-MM-DD)
    issue_resolved_confirmed INTEGER DEFAULT 0, -- 1: Đã xác nhận với KS sự cố giải quyết dứt điểm
    sys_created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. BẢNG CÁC CHUYẾN TÁC NGHIỆP HIỆN TRƯỜNG & CHỨNG TỪ CHUNG (TẦNG 2)
CREATE TABLE IF NOT EXISTS site_visits (
    visit_id INTEGER PRIMARY KEY AUTOINCREMENT,
    case_id TEXT NOT NULL,
    visit_seq INTEGER NOT NULL,            -- Thứ tự chuyến đi trong Case (#1, #2, #3...)
    visit_type TEXT NOT NULL,              -- 'CALIBRATE_ONLY', 'TEMP_TL_DISPATCH', 'OOW_REPAIR', 'RMA_FINAL_REPLACE'
    
    onsite_date TEXT NOT NULL,             -- Ngày thực tế kỹ sư xuống site / remote (YYYY-MM-DD)
    engineer_note TEXT,                    -- Ghi chú kết quả của chuyến đi
    
    -- Chứng từ & Lệnh NAV gộp chung theo chuyến đi
    nav_tl_created_date TEXT,              -- Ngày tạo lệnh TL / Transfer trên NAV
    huy_posted_tl INTEGER DEFAULT 0,       -- 1: Huy đã post phiếu TL trên NAV
    huy_posted_tl_date TEXT,               -- Ngày thực tế Huy post (YYYY-MM-DD)
    
    bbbg_issued_date TEXT,                 -- Ngày làm BBBG giao part cho kỹ sư
    bbbg_stamped_returned INTEGER DEFAULT 0, -- 1: Đã thu hồi BBBG có ký tên & đóng mộc của Site
    bbbg_returned_date TEXT,               -- Ngày thực tế nhận lại BBBG có mộc
    
    sr_collected INTEGER DEFAULT 0,        -- 1: Đã thu thập Service Report (SR) của chuyến đi này
    sr_collected_date TEXT,                -- Ngày thực tế nhận SR
    
    FOREIGN KEY(case_id) REFERENCES cases(case_id)
);

-- 3. BẢNG CHI TIẾT TỪNG LINH KIỆN TRONG CHUYẾN ĐI & LUỒNG RMA (TẦNG 3)
CREATE TABLE IF NOT EXISTS visit_part_items (
    item_id INTEGER PRIMARY KEY AUTOINCREMENT,
    visit_id INTEGER NOT NULL,
    case_id TEXT NOT NULL,
    
    -- Thông tin linh kiện mang đi
    out_part_no TEXT NOT NULL,             -- Mã Part chuẩn (đã kiểm chứng qua CSV NAV)
    out_serial_no TEXT NOT NULL,           -- Serial No của linh kiện mang đi
    part_outcome TEXT NOT NULL,            -- 'PENDING', 'UNUSED_RETURN', 'INSTALLED_CLAIM', 'OOW_UNFIXABLE', 'RMA_FINAL'
    
    -- Nhánh 1: Mang đi dự phòng/thử nghiệm nhưng KHÔNG DÙNG (UNUSED_RETURN)
    unused_returned_wh INTEGER DEFAULT 0,  -- 1: Kỹ sư đã trả linh kiện lành về lại kho chính
    unused_wh_date TEXT,                   -- Ngày thực tế trả kho
    
    -- Nhánh 2: Đúng bệnh, lắp vào máy khách & tháo xác hư đem về (INSTALLED_CLAIM / OOW)
    in_faulty_sn TEXT,                     -- Serial No của xác linh kiện hư tháo từ site
    faulty_returned_wh INTEGER DEFAULT 0,  -- 1: Đã thu hồi xác hư về kho (chờ WRR hoặc sửa chữa)
    faulty_wh_date TEXT,                   -- Ngày thực tế xác hư nhập kho & lưu kho
    
    -- Tiến độ Claim Bảo hành với Hãng (Dành riêng cho linh kiện này)
    claim_raised_date TEXT,                -- Ngày kỹ sư raise mail claim lên hãng
    vendor_status TEXT DEFAULT 'NONE',     -- 'NONE', 'WAITING_VENDOR', 'APPROVED', 'REJECTED'
    vendor_decision_date TEXT,             -- Ngày hãng phản hồi Approved / Rejected
    
    -- Hậu cần ERP (NAV) khi vendor_status == 'APPROVED'
    is_refurbished INTEGER DEFAULT 0,      -- 1: Linh kiện hãng trả về là hàng Refurbished
    zcl_item_code TEXT,                    -- Mã item ZCL- trên NAV (do Thái tạo)
    zcl_transferred INTEGER DEFAULT 0,     -- 1: Đã transfer NAV item ZCL về kho ZCL
    zcl_transfer_date TEXT,
    
    faulty_posted_huy INTEGER DEFAULT 0,   -- 1: Đã mail Huy post faulty item
    wrr_cc_hoa INTEGER DEFAULT 0,          -- 1: Đã CC Hoa làm thủ tục WRR
    wrr_mail_date TEXT,
    
    -- Xử lý đặc thù khi is_refurbished == 1
    clcus_worksheet_done INTEGER DEFAULT 0,-- 1: Đã tạo item worksheet gửi refurbish vào kho CLCUS cho site
    tl_new_part_pulled_back INTEGER DEFAULT 0, -- 1: Đã transfer part mới (ứng TL trước đó) về lại kho chính
    clcus_date TEXT,
    
    FOREIGN KEY(visit_id) REFERENCES site_visits(visit_id)
);

-- 4. BẢNG SỔ CÁI LUÂN CHUYỂN LINH KIỆN (PART LEDGER - TRACKING & ANALYTICS)
CREATE TABLE IF NOT EXISTS part_ledger (
    ledger_id INTEGER PRIMARY KEY AUTOINCREMENT,
    case_id TEXT NOT NULL,
    visit_id INTEGER NOT NULL,
    site TEXT NOT NULL,
    engineer_name TEXT NOT NULL,
    part_no TEXT NOT NULL,
    serial_no TEXT NOT NULL,
    zcl_code TEXT,
    usage_role TEXT NOT NULL,              -- 'TEMP_TL', 'RMA_OFFICIAL', 'FAULTY_PULL'
    is_refurbished INTEGER DEFAULT 0,
    transfer_out_date TEXT,                -- Ngày xuất khỏi kho
    transfer_in_date TEXT,                 -- Ngày thu hồi / nhập kho
    current_location TEXT NOT NULL,        -- 'WITH_ENG', 'AT_SITE', 'MAIN_WH', 'ZCL_WH', 'CLCUS_WH', 'WRR_HOA'
    is_settled INTEGER DEFAULT 0           -- 1: Đã tất toán xong vòng đời trong Case, 0: Đang treo
);

-- 5. BẢNG TỪ ĐIỂN TỰ HỌC MÃ ZCL- CỦA THÁI
CREATE TABLE IF NOT EXISTS zcl_mappings (
    original_part_no TEXT PRIMARY KEY,     -- Mã Part gốc trên NAV
    zcl_item_code TEXT NOT NULL,           -- Mã bắt đầu bằng "ZCL-" tương ứng
    updated_at TEXT
);

-- INDEX TỐI ƯU TỐC ĐỘ TRUY VẤN
CREATE INDEX IF NOT EXISTS idx_visits_case ON site_visits(case_id);
CREATE INDEX IF NOT EXISTS idx_items_visit ON visit_part_items(visit_id);
CREATE INDEX IF NOT EXISTS idx_ledger_part ON part_ledger(part_no, serial_no);
CREATE INDEX IF NOT EXISTS idx_ledger_settled ON part_ledger(is_settled);
```

---

## 4. ĐẶC TẢ LUỒNG NGHIỆP VỤ & LOGIC KIỂM CHỨNG (WORKFLOW & VALIDATION RULES)

### 4.1. Quy trình Đồng bộ CSV Tồn kho & Tự học Mã `ZCL-`
1. **Cấu trúc File CSV đầu vào:** File xuất từ hệ thống ERP (NAV) gồm 2 cột đầu cố định là `No.` và `Description`; toàn bộ các cột tiếp theo là mã các kho (`Location Code`) chứa số lượng tồn tương ứng.
2. **Logic Phân tích Ma trận Động (Dynamic Matrix Parsing):**
   - Đọc dòng tiêu đề (Line 1) để nạp danh sách tên kho động từ cột số 3 trở đi.
   - Xử lý chuỗi an toàn đối với trường hợp cột `Description` chứa dấu phẩy `,` nằm bên trong cặp dấu ngoặc kép `""`.
   - Khởi tạo bảng băm `std::unordered_map<std::string, size_t>` trên RAM để kiểm tra mã `No.` hợp lệ trong thời gian $O(1)$.
3. **Cơ chế Bán tự động cho Mã `ZCL-` (Do Thái tạo):**
   - Quy tắc bất biến: Mọi mã vật tư ZCL do Thái tạo luôn bắt đầu bằng tiền tố `ZCL-`, phần hậu tố phía sau thay đổi linh hoạt[cite: 1].
   - Khi cần gán mã ZCL cho một `out_part_no`:
     - **Bước 1:** Tra cứu trong bảng `zcl_mappings`. Nếu đã từng lưu trong lịch sử, tự động điền mã `ZCL-` đó.
     - **Bước 2:** Nếu chưa có trong `zcl_mappings`, quét toàn bộ dữ liệu CSV để lọc ra các dòng có cột `No.` bắt đầu bằng `ZCL-` khớp với từ khóa tìm kiếm để người dùng chọn nhanh bằng phím số.
     - **Bước 3:** Nếu Thái vừa tạo mã mới trên NAV (chưa kịp xuất CSV mới), người dùng nhập tay mã đó[cite: 1]. Hệ thống tự động xóa khoảng trắng thừa, chuyển thành chữ IN HOA, tự động chèn tiền tố `ZCL-` nếu người dùng quên gõ, và lập tức ghi cặp `(out_part_no, zcl_item_code)` vào bảng `zcl_mappings` để sử dụng tự động cho tương lai.

### 4.2. Sơ đồ Luồng Trạng Thái Nghiệp vụ (Workflow State Machine)

1. **Khởi tạo Case (`OPEN`):**
   - Kiểm tra đầy đủ thông số với kỹ sư: `Part No`, `Serial No`, `Service Item` (đánh dấu tạo mới nếu chưa có), `Branch`, `Hall`, `Site`, `Warranty`, `Initial Symptom`, `Initial Action`[cite: 1].
2. **Phân nhánh Không Thay Part (`need_part == 0`):**
   - Tạo chuyến tác nghiệp `CALIBRATE_ONLY` (Cân chỉnh, điều chỉnh/thiết lập từ xa hoặc tại site)[cite: 1].
   - Yêu cầu thu thập Service Report (`sr_collected = 1`) và xác nhận sự cố đã giải quyết để đóng Case[cite: 1].
3. **Phân nhánh Cần Thay Part - Hết Bảo Hành (`need_part == 1 && has_warranty == 0`):**
   - Tạo dịch vụ sửa chữa, transfer part hư cho kỹ sư trên NAV để theo dõi[cite: 1].
   - *Nếu kỹ sư sửa được:* Thay lại cho site $\rightarrow$ Thu thập SR & BBBG $\rightarrow$ Đóng Case[cite: 1].
   - *Nếu kỹ sư không sửa được:* Đánh dấu `OOW_UNFIXABLE` và theo dõi hướng giải quyết tiếp theo với kỹ sư[cite: 1].
4. **Phân nhánh Cần Thay Part - Còn Bảo Hành (`need_part == 1 && has_warranty == 1`):**
   - **Giai đoạn Chẩn đoán & Ứng Part (Lặp lại $N$ chuyến `TEMP_TL_DISPATCH`):**
     - Ưu tiên làm lệnh TL part trên NAV cho site trước $\rightarrow$ Gửi mail nhờ **Huy** post[cite: 1].
     - Làm BBBG (ký tên, đóng mộc) và xuất 1 hoặc nhiều part cho kỹ sư đi thay/thử tại site[cite: 1].
     - Đối với part mang đi thử nhưng không đúng bệnh (`UNUSED_RETURN`): Theo dõi kỹ sư trả lại kho chính.
     - Đối với part lắp đúng bệnh (`INSTALLED_CLAIM`): Thu hồi xác part hư về kho (để chuẩn bị làm WRR hoặc sửa chữa) và nhắc kỹ sư raise mail claim lên hãng[cite: 1].
   - **Giai đoạn Phản hồi từ Hãng (Vendor Decision):**
     - *Nếu Hãng Từ chối (`REJECTED`):* Chuyển sang luồng theo dõi xử lý với kỹ sư đối với linh kiện đã ứng TL[cite: 1].
     - *Nếu Hãng Phê duyệt (`APPROVED` - Yếu tố then chốt):*
       1. Kiểm tra/nhờ **Thái** tạo mã `ZCL-` mới (nếu chưa có) và thực hiện Transfer NAV item ZCL về kho `ZCL`[cite: 1].
       2. Gửi mail nhờ **Huy** post faulty item, đồng thời CC **Hoa** làm thủ tục WRR[cite: 1].
       3. *Nếu hàng hãng trả là hàng Refurbished (`is_refurbished == 1`):* Tạo Item Worksheet gửi hàng refurbish vào kho `CLCUS` cho site, đồng thời làm lệnh transfer rút cục part mới (đã ứng TL tạm cho khách ở chuyến trước) về lại kho chính[cite: 1].
       4. Tạo chuyến đi chốt hạ (`RMA_FINAL_REPLACE`): Làm BBBG cho kỹ sư đi thay chính thức part RMA, yêu cầu site đóng mộc, thu hồi BBBG có mộc lần cuối, lấy Service Report (SR) lần cuối và sắp xếp lưu kho phù hợp cho các part thu về[cite: 1].

### 4.3. Bộ Quy tắc Chốt chặn Đóng Case (Gatekeeper Algorithm)
Khi người dùng gọi lệnh `Close Case`, hệ thống chạy hàm kiểm tra toàn vẹn. Nếu vi phạm **bất kỳ** điều kiện nào dưới đây, lệnh đóng bị từ chối và hệ thống in ra danh sách các mục còn nợ[cite: 1]:
- `[Rule 1]` Chưa xác nhận với kỹ sư rằng sự cố tại site đã được giải quyết hoàn toàn (`issue_resolved_confirmed == 0`)[cite: 1].
- `[Rule 2]` Tồn tại chuyến đi (`site_visits`) chưa thu thập Service Report (`sr_collected == 0`)[cite: 1].
- `[Rule 3]` Tồn tại chuyến đi có xuất linh kiện nhưng Huy chưa post lệnh TL (`huy_posted_tl == 0`) hoặc chưa thu hồi được BBBG có đóng mộc của khách (`bbbg_stamped_returned == 0`)[cite: 1].
- `[Rule 4]` Tồn tại linh kiện mang đi dư (`UNUSED_RETURN`) chưa trả về kho (`unused_returned_wh == 0`), hoặc xác linh kiện hư (`INSTALLED_CLAIM`) chưa được kỹ sư đem về kho lưu trữ (`faulty_returned_wh == 0`)[cite: 1].
- `[Rule 5]` Tồn tại linh kiện đã được hãng `APPROVED` nhưng chưa hoàn tất chuỗi hậu cần NAV: chưa transfer kho ZCL (`zcl_transferred == 0`), chưa mail Huy post faulty & CC Hoa làm WRR (`faulty_posted_huy == 0 || wrr_cc_hoa == 0`), hoặc (nếu là hàng Refurbished) chưa đẩy vào CLCUS và chưa rút part TL tạm về lại kho (`clcus_worksheet_done == 0 || tl_new_part_pulled_back == 0`)[cite: 1].

---

## 5. ĐẶC TẢ TIỆN ÍCH VẬN HÀNH (OPERATIONAL UTILITIES)

### 5.1. Bộ Phân tích Ngày Tháng Thông minh (Smart Date Parser)
Mọi mốc thời gian nhập vào qua giao diện CLI đều đi qua hàm `parse_and_standardize_date(input)`:
- **Nhấn `Enter` (Chuỗi rỗng):** Tự động lấy ngày hiện tại của hệ thống (`YYYY-MM-DD`).
- **Nhập số âm (`-1`, `-2`, `-n`):** Tự động lùi $n$ ngày so với hôm nay (Ví dụ: Sáng thứ Hai nhập `-2` để ghi nhận sự việc diễn ra vào thứ Bảy).
- **Nhập ngắn gọn (`DD/MM` hoặc `DD-MM`):** Tự động ghép năm hiện tại và chuẩn hóa thành `YYYY-MM-DD`.
- **Kiểm tra Logic Dòng thời gian (Timeline Validation):** Ngăn chặn việc nhập mốc thời gian bước sau nhỏ hơn bước trước (Ví dụ: Ngày nhận BBBG có mộc không được nhỏ hơn ngày xuất BBBG).

### 5.2. Mẫu Sinh Email Tự động vào Clipboard (Clipboard Templates)
Hệ thống tích hợp sẵn các mẫu văn bản chuẩn hóa để tránh sai sót mã số khi giao tiếp nội bộ[cite: 1]:
1. **Mẫu Mail 1 - Gửi Huy (Post TL):**
   > *"Chào Huy, nhờ Huy post giúp lệnh TL trên NAV cho Site: [site] ([branch] - [hall]) nhé. Mã Case: [case_id] | Danh sách Part: [list_of_parts_and_sn]. Cảm ơn Huy!"*[cite: 1]
2. **Mẫu Mail 2 - Gửi Thái (Tạo mã ZCL):**
   > *"Chào Thái, nhờ Thái tạo mới giúp mã item ZCL trên NAV cho linh kiện bảo hành của Case [case_id] (Theo loop mail giữa kỹ sư [engineer_name] và hãng): Mã gốc: [out_part_no] - [description]. Cảm ơn Thái!"*[cite: 1]
3. **Mẫu Mail 3 - Gửi Huy & CC Hoa (Post Faulty & Làm WRR):**
   > *"Chào Huy, nhờ Huy post giúp faulty item trên NAV cho Case [case_id] | Site: [site] | Part No: [out_part_no] | Faulty S/N: [in_faulty_sn] | Mã ZCL: [zcl_item_code]. @Hoa: Mình CC Hoa nắm thông tin để tiến hành làm thủ tục WRR cho linh kiện lỗi này nhé. Cảm ơn hai bạn!"*[cite: 1]

---

## 6. HƯỚNG DẪN BIÊN DỊCH & BÀN GIAO CHO NGƯỜI KẾ NHIỆM (DEPLOYMENT & HANDOVER)

### 6.1. Quy trình Biên dịch (Build Instructions)
Dự án không phụ thuộc vào thư viện ngoài hệ thống. Để biên dịch ra file thực thi độc lập trên Windows (sử dụng MinGW-w64 / GCC hoặc MSVC):
```bash
# Biên dịch kèm mã nguồn SQLite Amalgamation (sqlite3.c)
g++ -std=c++17 -O2 src/main.cpp third_party/sqlite3.c -o bin/rma_tracker.exe -static
```

### 6.2. Quy trình Sao lưu & Chuyển giao (Handover Procedure)
Khi bàn giao công việc cho người kế nhiệm hoặc di chuyển sang máy tính khác:
1. Toàn bộ trạng thái hệ thống nằm gọn trong thư mục `bin/` gồm 3 file: `rma_tracker.exe`, `rma_tracker.db` và `nav_inventory.csv`.
2. Chỉ cần sao chép thư mục này sang máy mới là có thể tiếp tục theo dõi các Case đang mở mà không cần cài đặt lại bất kỳ phần mềm hay cơ sở dữ liệu nào.