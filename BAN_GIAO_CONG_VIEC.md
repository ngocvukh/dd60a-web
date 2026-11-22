# BIÊN BẢN BÀN GIAO CÔNG VIỆC — DỰ ÁN CẦU NỐI IN NHIỆT BORGWALDT DD60A

**Ngày cập nhật bàn giao:** 22/11/2026  
**Repository GitHub:** `https://github.com/ngocvukh/dd60a-web`  
**Link Web QR (GitHub Pages):** `https://ngocvukh.github.io/dd60a-web/`  
**Firebase Realtime Database:** `https://dd60a-244ce-default-rtdb.asia-southeast1.firebasedatabase.app` *(Singapore `asia-southeast1`)*

---

## 1. Các hạng mục đã hoàn thành & nghiệm thu

| Hạng mục | Trạng thái | Chi tiết |
| :--- | :---: | :--- |
| **Firebase Realtime Database** | Hoàn tất | Đã tạo database, cấu hình Rules đọc/ghi lâu dài cho `/dd60a` và `/dd60a_presets` (không giới hạn 30 ngày). |
| **Web cấu hình qua QR** | Hoàn tất | Deploy [index.html](file:///d:/DD%2060%20A/index.html) + [qrcode.js](file:///d:/DD%2060%20A/qrcode.js) lên GitHub Pages (`https://ngocvukh.github.io/dd60a-web/`), hỗ trợ cả nhập tay và sinh mã QR tự động cho từng phân loại để quét đẩy nhanh lên ESP32. |
| **Ảnh nhãn QR để in** | Hoàn tất | Tạo sẵn file [QR_DD60A.png](file:///d:/DD%2060%20A/QR_DD60A.png) (`680 x 830 px`) gồm mã QR Web + thông tin Wi-Fi `DD60A-Setup` (`12345678`, `http://192.168.4.1`). |
| **Firmware ESP32** | Hoàn tất & Đã nạp | Đã biên dịch (`1220877 bytes`) và nạp [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino) lên ESP32 qua `COM9`, hỗ trợ OLED 0.91" (`0x3C`), LED báo Wi-Fi (`GPIO 25`/`26`) và tự reset cấu hình sau mỗi loạt in thật. |
| **Phần cứng & Sơ đồ đấu dây** | Đã test thông mạch | Đã cập nhật sơ đồ đảo chân thực tế cho cả **MAX3232 #1** (`RXD -> GPIO 16`, `TXD -> GPIO 17`) và **MAX3232 #2** (`RXD -> GPIO 18`, `TXD -> GPIO 19`). |
| **Công cụ test PowerShell** | Hoàn tất | Tạo sẵn [test_dd60a_send.ps1](file:///d:/DD%2060%20A/test_dd60a_send.ps1) (giả lập DD60A `2400 7-N-2`) và [test_rs232_monitor.ps1](file:///d:/DD%2060%20A/test_rs232_monitor.ps1) (giám sát đầu ra RS-232). |
| **Tài liệu hướng dẫn** | Hoàn tất | Đã tách gọn thành 4 hạng mục trong `huong_dan/` và mục lục tại [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md). |

---

## 2. Tính năng đã lập trình trong phiên làm việc

1. **Xử lý số liệu & Định dạng phiếu in (100% ASCII, tiết kiệm giấy 58mm):**
   - Nhận dữ liệu từ DD60A (`2400 baud, SERIAL_7N2`, khung `0x02..0x03`, `XXXXXX / 10000.0` mm).
   - Quy đổi `% = (giaTriDo / (chuVi / PI)) * 100` và làm tròn chuẩn toán học cho cả `mm` và `%` theo `dec` (`0`–`4`).
   - Khi chưa có chu vi từ Web (`circ` là `null` / `<= 0`), in `LOAI: null` (hoặc `LOAI: <Loại> (null)`) và **không tính `%`** (chỉ in `mm` + `COEFFICIENT OF VARIATION (V.C.)`).
   - In Font B, thu hẹp giãn dòng, ghép **2 cột** cho danh sách mẫu (`01:5.79|74.24%`), in `MAX`/`MIN`/`MEAN`/`STD` cùng định dạng `mm|%`, và dòng cuối `COEFFICIENT OF VARIATION (V.C.): ...%` trước khi cắt giấy (`GS V 66 0`).
2. **Gom mẫu theo loạt & kiểm tra STT:**
   - Gom toàn bộ mẫu đo (`MAX_SAMPLES = 150`) và chỉ in trọn phiếu khi nhận gói kết thúc `FF05` (có timeout dự phòng `800 ms` sau `FF01`–`FF04`).
   - Nếu nhận `0000` mới hoặc `sampleNum <= lastSampleNum` khi loạt cũ chưa kết thúc, ESP32 tự in phiếu cũ kèm cảnh báo `! CANH BAO: Mau da do chua duoc in truoc !` rồi mới lưu mẫu mới.
   - Sau mỗi lần in xong một loạt đo thật (`printFullBatch(false)`), ESP32 tự động reset `sample`, `name`, `type`, `subType`, `circ`, `isManualCirc` về `null` trên RAM, NVS, màn hình OLED (`Loai: null` / `null`) và đồng bộ `PATCH` xóa trên Firebase `/dd60a.json` (chỉ giữ lại `dec`; không reset khi bấm `In thử` từ Web).
3. **Captive Portal `DD60A-Setup` (`192.168.4.1`) & Lưu bền vững NVS:**
   - Đổi Wi-Fi (kèm quét Wi-Fi xung quanh không xung đột Core 0) và chỉnh cấu hình máy in Citizen (đủ 8 mức Baud `1200`–`115200`, `7/8 bit`, `Parity None/Even/Odd`, `X-ON/X-OFF / None`) trực tiếp trên Web nội bộ mà không cần nạp lại code.
   - Mặc định cổng máy in đặt sẵn `2400 bps, 7-N-1, X-ON/X-OFF` khớp với tờ Self-Test của Citizen CT-S310II.
   - Toàn bộ cấu hình Wi-Fi, cổng máy in và thông số bản in lưu trong Flash NVS (`Preferences`), **không bị mất khi mất điện** và vẫn in Offline bình thường khi rớt Wi-Fi.
4. **Web cấu hình (`index.html` + `qrcode.js`) — Giữ nguyên 100% firmware ESP32:**
   - Tự động bỏ dấu tiếng Việt (`removeAccents()`), cho phép để trống phân loại/chu vi, nhập tay chu vi, và quản lý danh sách phân loại (thêm/sửa/xóa vĩnh viễn với `{ initialized: true, items: [...] }` + nút `Xóa tất cả`).
   - **Sinh mã QR tự động & Quét đẩy nhanh:** Mỗi khi thêm/cập nhật phân loại (hoặc bấm `Mã QR` trên từng dòng), Web tự tạo tem QR gắn nhãn (`Loại`, `Phân loại`, `Chu vi mm`), cho phép tải ảnh PNG, in tem lẻ hoặc in tất cả QR khổ A4. Khi người đo quét bằng Camera/Zalo (`?auto=1&type=...&subType=...&circ=...`), Web tự động đặt trống `sample: null` và `name: null`, đẩy cấu hình lên `/dd60a.json` cho ESP32 tự cập nhật trong ~3 giây, rồi tự động chuyển hướng sang `https://www.google.com` (truy cập trực tiếp link gốc vẫn dùng bình thường giao diện Web nhập tay).

---

## 3. Kết quả kiểm thử đã xác minh

- **Kiểm thử ESP32 + Firebase + NTP (`20/11/2026`, `COM10`):**
  - ESP32 kết nối Wi-Fi thành công, đồng bộ giờ thực tế NTP, đọc cấu hình từ Web qua Firebase, tự động reset cờ `"testPrint": false`.
- **Kiểm thử phần cứng & toàn tuyến RS-232 (`22/11/2026`, `COM9` & `COM11`):**
  - **OLED 0.91" (`128x32`):** Nhận địa chỉ I2C `0x3C` (`SDA=21`, `SCL=22`), hiển thị thông số bình thường.
  - **Đầu ra RS-232 (`ESP32 -> MAX3232 #2 -> COM11`):** Nhận đủ `427 bytes` phiếu in thử ở `9600, 7-N-1` (gồm lệnh `ESC @` và cắt giấy `GS V 66 0`).
  - **Đầu vào RS-232 (`COM11 -> MAX3232 #1 -> ESP32 -> COM9`):** Đẩy 10 khung giả lập DD60A (`2400, 7-N-2`, `0000`–`0004` & `FF01`–`FF05`), ESP32 giải mã, ghép cấu hình Firebase (`MAU : Test`, `LOAI: Dau loc - Nhap tay`, `DO: Vugggg`), tính `MAX/MIN/MEAN/STD/V.C. (0.75%)` và xuất phiếu hoàn chỉnh qua `COM9`.

---

## 4. Việc cần làm tiếp theo (triển khai tại hiện trường)

1. Trên Web QR (`https://ngocvukh.github.io/dd60a-web/`), đổi lại chu vi thực tế (ví dụ `24.15` mm) thay cho giá trị test `2222222.01` mm.
2. Kiểm tra lại cấu hình Serial máy in tại Captive Portal `DD60A-Setup` (`http://192.168.4.1`) đang ở **`2400 bps | 7-N-1 | X-ON/X-OFF`** khớp với tờ Self-Test của **Citizen CT-S310II**.
3. Cắm thực tế vào máy **Borgwaldt DD60A** (cổng 35) và **Citizen CT-S310II**, thực hiện test đo thật và cắt giấy theo [04_KIEM_THU_VA_XU_LY_SU_CO.md](file:///d:/DD%2060%20A/huong_dan/04_KIEM_THU_VA_XU_LY_SU_CO.md).
4. In file [QR_DD60A.png](file:///d:/DD%2060%20A/QR_DD60A.png) dán tại vị trí đo.
5. *(Tùy chọn)* Khi có điện trở `220Ω`–`330Ω` thì lắp thêm 2 đèn LED báo Wi-Fi vào `GPIO 25` / `GPIO 26` (không bắt buộc vì đã có màn hình OLED).
