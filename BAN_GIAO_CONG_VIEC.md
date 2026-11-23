# BIÊN BẢN BÀN GIAO CÔNG VIỆC — DỰ ÁN CẦU NỐI IN NHIỆT BORGWALDT DD60A

**Ngày cập nhật bàn giao:** 23/11/2026  
**Repository GitHub:** `https://github.com/ngocvukh/dd60a-web`  
**Link Web QR (GitHub Pages):** `https://ngocvukh.github.io/dd60a-web/`  
**Firebase Realtime Database:** `https://dd60a-244ce-default-rtdb.asia-southeast1.firebasedatabase.app` *(Singapore `asia-southeast1`)*

---

## 1. Các hạng mục đã hoàn thành & nghiệm thu thực tế

| Hạng mục | Trạng thái | Chi tiết |
| :--- | :---: | :--- |
| **Firebase Realtime Database** | Hoàn tất | Đã tạo database, cấu hình Rules đọc/ghi lâu dài cho `/dd60a` và `/dd60a_presets` (không giới hạn 30 ngày). |
| **Web cấu hình qua QR** | Hoàn tất | Deploy [index.html](index.html) + [qrcode.js](qrcode.js) lên GitHub Pages (`https://ngocvukh.github.io/dd60a-web/`), hỗ trợ cả nhập tay và sinh mã QR tự động cho từng phân loại để quét đẩy nhanh lên ESP32. |
| **Ảnh nhãn QR & Sơ đồ cáp** | Hoàn tất | Tạo sẵn [QR_DD60A.png](QR_DD60A.png) (`680 x 830 px`) và ảnh sơ đồ đấu cáp [so_do_dau_day_25_9pin.png](so_do_dau_day_25_9pin.png). |
| **Firmware ESP32** | Hoàn tất & Đã nạp | Đã biên dịch (`1009893 bytes`, core `esp32@2.0.17`) và nạp [dd60a_bridge.ino](dd60a_bridge/dd60a_bridge.ino) lên ESP32 (`COM11`), hỗ trợ OLED 0.91" (`0x3C`), LED báo Wi-Fi (`GPIO 25`/`26`), log debug trực tiếp `[DD60A RAW]`/`[DD60A PKT]`, gom trọn 1 phiếu duy nhất và tự reset cấu hình sau mỗi loạt in thật. |
| **Đấu nối Cáp RS-232 & Phần cứng** | Đã nghiệm thu trên máy thật | Đã xác minh và đấu nối hoàn chỉnh từ cổng 35 (`25-pin`) của **Borgwaldt DD60A** qua **MAX3232 #1** (`RXD -> GPIO 16`, `TXD -> GPIO 17`) sang **ESP32** và từ **MAX3232 #2** (`RXD -> GPIO 18`, `TXD -> GPIO 19`) ra máy in **Citizen CT-S310II** (`9-pin`). |
| **Công cụ test PowerShell** | Hoàn tất | Tạo sẵn [test_dd60a_send.ps1](test_dd60a_send.ps1) (giả lập DD60A) và [test_rs232_monitor.ps1](test_rs232_monitor.ps1) (giám sát đầu ra RS-232). |
| **Tài liệu hướng dẫn** | Hoàn tất | Đã cập nhật đầy đủ 4 hạng mục trong `huong_dan/` và mục lục tại [HUONG_DAN_TRIEN_KHAI.md](HUONG_DAN_TRIEN_KHAI.md). |

---

## 2. Chi tiết kỹ thuật & Các cập nhật đã hoàn thiện

### 2.1. Sơ đồ cáp RS-232 thực tế giữa Borgwaldt DD60A (`25-pin`) và MAX3232 #1 (`9-pin`)
Khi chuyển từ việc cắm thẳng máy in **Citizen CT-S310II (`9-pin`)** sang cắm vào **Module MAX3232 #1 (`9-pin`)**, có 2 khác biệt vật lý đã được xử lý triệt để trên đầu cáp `9-pin` của sợi `25-9` (ghép nối tiếp sợi `9-9`):
1. **Đảo chéo chân dữ liệu `2 ↔ 3`:**
   - Máy in Citizen (`9-pin`) nhận dữ liệu (`RXD`) ở **Chân 2**, trong khi module `MAX3232 #1` nhận dữ liệu (`RXD` vào `GPIO 16`) ở **Chân 3**.
   - Do đó cần đảo chéo để **`DB25 Chân 2 (TXD DD60A)` ── thông ── `DB9 Chân 3 (RXD MAX3232 #1)`** và **`DB25 Chân 3 (RXD DD60A)` ── thông ── `DB9 Chân 2 (TXD MAX3232 #1)`**.
2. **Nối chập vòng bắt tay phần cứng `Chân 4 – Chân 6 – Chân 7` trên đầu `9-pin`:**
   - Trên cáp `25-9` nguyên bản:
     - `DB9 Chân 4` nối với `DB25 Chân 6` (**`DSR`** — ngõ vào DD60A chờ thiết bị nhận sẵn sàng).
     - `DB9 Chân 6` nối với `DB25 Chân 20` (**`DTR`** — ngõ ra DD60A cấp áp báo sẵn sàng).
     - `DB9 Chân 7` nối với `DB25 Chân 5` (**`CTS`** — *Clear To Send*, ngõ vào DD60A cho phép truyền dữ liệu).
   - Khi cắm vào Citizen, máy in tự cấp áp dương ra `Chân 4 (DTR)` và `Chân 7 (RTS)`. Khi cắm vào `MAX3232 #1` (vốn chỉ nối 3 chân `2, 3, 5` và bỏ trống `4, 6, 7, 8`), việc **hàn nối chập 3 chân `4 – 6 – 7` lại với nhau** sẽ lấy chính áp `DTR (Chân 20 DB25)` của DD60A cấp vòng ngược lại cho cả **`DSR (Chân 6 DB25)`** lẫn **`CTS (Chân 5 DB25)`**, giúp DD60A luôn mở khóa truyền dữ liệu.
3. **Đường Mass chung (`GND`):** `DB25 Chân 7` ── thông ── `DB9 Chân 5`.

### 2.2. Xử lý số liệu, Gom trọn 1 phiếu in & Tự động Reset
1. **Giao tiếp UART1 (`Serial1`):**
   - Cấu hình `Serial1.begin(2400, SERIAL_7N1, DD60A_RX_PIN, DD60A_TX_PIN)` (tương thích cả nguồn phát `7-N-1` lẫn `7-N-2`), lọc `0x7F` lấy chuẩn ASCII 7-bit trong khung `0x02 (SOT) ... 0x03 (EOT)`.
   - Bổ sung log debug trực tiếp `[DD60A RAW]` (in mã HEX + ký tự ASCII của từng byte chạm tới `GPIO 16`) và `[DD60A PKT]` trên cổng USB `Serial` (`115200 baud`) để dễ dàng giám sát/chẩn đoán qua máy tính bất cứ lúc nào.
2. **Gom trọn 1 phiếu duy nhất cho mỗi loạt đo (`6000 ms` fallback timeout):**
   - Thực tế khi bấm kết thúc loạt đo trên **Borgwaldt DD60A**, máy vừa điều khiển đầu in kim cơ khí vừa phát lần lượt các gói thống kê `FF01` (`MAX`) → `FF02` (`MIN`) → `FF03` (`MEAN`) → `FF04` (`STD`) → `FF05` (`V.C.`), mỗi gói cách nhau từ **`2.1 giây` đến `3.3 giây`**.
   - Đã nâng thời gian chờ dự phòng (`lastStatPacketTime`) từ `800 ms` lên **`6000 ms` (6 giây)**: đảm bảo ESP32 không bao giờ cắt phiếu sớm giữa chừng, mà gom đủ toàn bộ mẫu đo (`0001`–`0150`) cùng cả 5 chỉ số `FF01`–`FF05` rồi **in ra đúng 1 tờ phiếu duy nhất** ngay khi nhận được `FF05` (và vẫn tự động in sau 6 giây nếu bài đo 1 mẫu dừng ở `FF03`).
3. **Định dạng phiếu in tiết kiệm giấy 58mm & Tự động xóa cấu hình sau khi in:**
   - In 100% ASCII không dấu, Font B, thu hẹp giãn dòng, ghép **2 cột** cho danh sách mẫu (`01:5.79|74.24%` hoặc `01:5.400mm` khi chưa nhập chu vi `circ`), in `MAX`/`MIN`/`MEAN`/`STD`/`COEFFICIENT OF VARIATION (V.C.)` và tự động cắt giấy (`GS V 66 0`).
   - Sau mỗi lần in xong một loạt đo thật (`printFullBatch(false)`), ESP32 tự động reset `sample`, `name`, `type`, `subType`, `circ`, `isManualCirc` về `null` trên RAM, NVS, màn hình OLED (`Loai: null` / `null`) và đồng bộ `PATCH` lên Firebase `/dd60a.json` (chỉ giữ lại `dec`; không reset khi bấm `In thử` từ Web).
4. **Captive Portal `DD60A-Setup` (`192.168.4.1`) & Web cấu hình QR:**
   - Đổi Wi-Fi 2.4GHz và chỉnh thông số cổng máy in Citizen (`1200`–`115200 bps`, `7/8 bit`, `Parity`, `X-ON/X-OFF`) trực tiếp tại `http://192.168.4.1` lưu bền vững vào Flash NVS.
   - Hỗ trợ quét mã QR phân loại (`?auto=1&type=...&subType=...&circ=...`) để đẩy nhanh chu vi lên Firebase trong ~3 giây rồi tự chuyển hướng sang Google.

---

## 3. Nhật ký kiểm thử & Nghiệm thu thực tế

- **Kiểm thử ESP32 + Firebase + NTP (`20/11/2026`, `COM10`):**
  - ESP32 kết nối Wi-Fi, đồng bộ giờ NTP, đọc cấu hình từ Web qua Firebase, tự động reset cờ `"testPrint": false`.
- **Kiểm thử phần cứng & giả lập RS-232 (`22/11/2026`, `COM9` & `COM11`):**
  - OLED 0.91" (`128x32`) nhận địa chỉ I2C `0x3C` (`SDA=21`, `SCL=22`); xác minh thông mạch 2 chiều với script giả lập.
- **Nghiệm thu thực tế trên máy Borgwaldt DD60A & Citizen CT-S310II (`23/11/2026`, `COM11`):**
  - Đấu cáp thực tế `25-9` + `9-9` (đảo `2 ↔ 3`, nối chập `4 – 6 – 7`, giữ `5 – 7` GND), nạp firmware mới (`SERIAL_7N1`, timeout `6000 ms`).
  - Thực hiện đo liên tiếp **3 loạt mẫu thật trên Borgwaldt DD60A** (`12:29`, `12:33`, `12:35`), kết quả log thu được trên `COM11` và in ra máy in nhiệt **Citizen CT-S310II**:
    - **Loạt 1 (`12:29` — 3 mẫu):** Nhận đủ `0000`, `0001 840700` (`84.070mm`), `0002 840600` (`84.060mm`), `0003 840400` (`84.040mm`), `FF01` (`MAX: 84.070mm`), `FF02` (`MIN: 84.040mm`), `FF03` (`MEAN: 84.057mm`), `FF04` (`STD: 0.0147mm`), `FF05` (`V.C.: 0.02%`) → **In trọn vẹn 1 phiếu duy nhất**, tự cắt giấy và đồng bộ reset Firebase.
    - **Loạt 2 (`12:33` — 3 mẫu):** Nhận đủ `0000`, `0001 055000` (`5.500mm`), `0002 054300` (`5.430mm`), `0003 053900` (`5.390mm`), `FF01`–`FF05` (`MAX: 5.500mm`, `MIN: 5.390mm`, `MEAN: 5.440mm`, `STD: 0.0557mm`, `V.C.: 1.02%`) → **In trọn vẹn 1 phiếu duy nhất**, tự cắt giấy và đồng bộ reset Firebase.
    - **Loạt 3 (`12:35` — 2 mẫu):** Nhận đủ `0000`, `0001 054000` (`5.400mm`), `0002 053700` (`5.370mm`), `FF01`–`FF05` (`MAX: 5.400mm`, `MIN: 5.370mm`, `MEAN: 5.385mm`, `STD: 0.0212mm`, `V.C.: 0.39%`) → **In trọn vẹn 1 phiếu duy nhất**, tự cắt giấy và đồng bộ reset Firebase.
