# BIÊN BẢN BÀN GIAO DỰ ÁN: CẦU NỐI IN NHIỆT THÔNG MINH CHO BORGWALDT DD60A

**Mục đích tài liệu:** Cung cấp đầy đủ ngữ cảnh kỹ thuật, kết quả thực nghiệm đã xác minh, kiến trúc hệ thống và mã nguồn hiện tại để tiếp tục phát triển, đóng gói và triển khai trên **Antigravity**.

---

## 1. Mục tiêu dự án
Thay thế cụm máy in kim tích hợp (đã cũ, mờ mực) trên máy đo tỷ trọng/độ cứng thuốc lá **Borgwaldt DD60A** bằng máy in hóa đơn nhiệt **Citizen CT-S310II** với các yêu cầu:
1. **Không dùng máy tính (PC)** tại vị trí đo.
2. **Tự động định dạng phiếu in** giống mẫu máy in kim của hãng (`SAMPLE`, `DATE`, `NAME`, từng mẫu `01`–`150`, `MAX`, `MIN`, `MEAN`, `STD`, `V.C.`) và **tự động cắt giấy** sau mỗi loạt đo.
3. **Tùy chỉnh thông số bản in linh hoạt qua mã QR (hỗ trợ 4G/5G hoặc bất kỳ Wi-Fi nào):** Do xưởng có nhiều loại điếu và đầu lọc khác nhau, người vận hành chỉ cần quét 1 mã QR cố định để mở trang web cấu hình, chọn nhanh loại điếu/đầu lọc hoặc nhập thông tin mẫu rồi đẩy cấu hình xuống máy in qua Internet.

---

## 2. Thông số kỹ thuật & Kết quả kiểm chứng thực tế

### 2.1. Thiết bị nguồn: Borgwaldt DD60A (Cổng RS-232 số 35)
* **Cấu hình Serial (theo tài liệu mục 4.4):** `2400 baud`, `7 data bits`, `No parity`, `2 stop bits` (`SERIAL_7N2`), định dạng ASCII.
* **Cấu trúc gói tin (đã xác minh qua Hex Dump thực tế trên Citizen):**
  * Định dạng: `[0x02 (SOT)]` + `4 ký tự mã` + `0x20 (SP)` + `6 chữ số giá trị` + `[0x03 (EOT)]`.
  * Ví dụ Hex thực tế: `02 30 30 30 33 20 38 34 31 31 30 30 03` -> `0003 841100`.
  * **Lưu ý quan trọng:** DD60A **không gửi kèm ký tự xuống dòng (`CR`/`LF`)**. Khi cắm trực tiếp vào máy in nhiệt, dữ liệu bị dính liền nhau và phần đuôi (`FF04`, `FF05`) bị kẹt trong bộ nhớ đệm do chưa đủ 48 ký tự/dòng. Đây là lý do bắt buộc dùng ESP32 làm cầu nối.
* **Bảng mã gói tin:**
  * `0000 000000`: Bắt đầu loạt đo mới.
  * `0001` đến `0150`: Kết quả từng mẫu đo lẻ.
  * `FF01`: Giá trị lớn nhất (`MAX`).
  * `FF02`: Giá trị nhỏ nhất (`MIN`).
  * `FF03`: Giá trị trung bình (`MEAN`).
  * `FF04`: Độ lệch chuẩn (`STD`).
  * `FF05`: Hệ số biến thiên (`V.C.`) -> **Tín hiệu kết thúc loạt đo để kích hoạt cắt giấy**.
* **Quy tắc dấu thập phân & công thức quy đổi `%` (đã đối chiếu khớp giữa phiếu máy in kim và dữ liệu RS-232):**
  * Chuỗi 6 chữ số `XXXXXX` luôn đặt dấu chấm thập phân **sau 2 chữ số đầu tiên** (`XX.XXXX`), đơn vị gốc là `mm` (hoặc giá trị % sẵn đối với `FF05`).
  * **Công thức quy đổi sang `%` dựa trên Chu vi (`circ`, đơn vị `mm`) gửi từ Web:**
    $$\% = \frac{\text{Giá trị đo (mm)}}{\text{Chu vi (mm)} / \pi} \times 100$$
  * **Quy tắc làm tròn & hiển thị trên phiếu in:**
    * Làm tròn chuẩn toán học (`round`) cho cả `mm` và `%` theo cùng số chữ số thập phân `dec` cấu hình từ Web (mặc định `2` số lẻ).
    * **Từng mẫu đo (`01`–`150`):** In cả 2 giá trị trên mỗi dòng, ví dụ: `01   : 5.79 mm | 74.24 %`.
    * **`MAX` (`FF01`), `MIN` (`FF02`), `MEAN` (`FF03`), `STD` (`FF04`):** Đều quy đổi sang `%` theo công thức trên (`MAX/MIN/MEAN` dùng `dec` số lẻ; `STD` giữ `4` số lẻ).
    * **`V.C.` (`FF05`):** Giữ nguyên giá trị `%` từ máy (`2` số lẻ) -> kích hoạt cắt giấy.

### 2.2. Thiết bị đích: Citizen CT-S310II
* **Cấu hình RS-232:** Mặc định ESP32 dùng **`2400 baud`, `7-N-1` (`SERIAL_7N1`), `X-ON/X-OFF`** khớp sẵn với cấu hình hiện tại trên tờ Self-Test của Citizen CT-S310II (không cần chỉnh lại máy in). Có thể đổi đầy đủ **8 mức Baud (`1200`–`115200`)**, **`7 bit / 8 bit`**, **`Parity (None/Even/Odd)`** và **`Handshaking (X-ON/X-OFF / None)`** ngay trên trang nội bộ `DD60A-Setup` (`http://192.168.4.1`).
* **Giao thức:** Chuẩn ESC/POS (100% ASCII không dấu để đảm bảo tương thích tuyệt đối cả ở chế độ 7-bit và 8-bit).
  * Khởi tạo máy in (`ESC @`): `0x1B 0x40`
  * Tiết kiệm giấy khổ 58mm: Font B (`0x1B 0x4D 0x01`), thu hẹp giãn dòng (`0x1B 0x33 0x14`), in 2 cột.
  * Đẩy giấy và cắt giấy (`GS V 66 0`): `0x1D 0x56 0x42 0x00`
* **Cơ chế gom gói tin (Batch Buffering & Xử lý STT mẫu):** ESP32 gom toàn bộ từng mẫu (`0001`–`0150`) và thống kê (`FF01`–`FF04`) vào bộ nhớ đệm, chỉ in toàn bộ phiếu và cắt giấy khi xuất hiện gói kết thúc **`FF05`** (kèm timeout dự phòng `800 ms` nếu máy không gửi `FF05`). Nếu STT mẫu mới `<=` STT mẫu trước (hoặc nhận `0000` mới khi loạt cũ chưa kết thúc), ESP32 tự in phiếu cũ kèm cảnh báo `! CANH BAO: Mau da do chua duoc in truoc !` rồi mới bắt đầu loạt mới.
* **Dây cáp hiện có:** Đã xác minh thông suốt 100% ở bài test Hex Dump (không bị ngược chân TX/RX).

---

## 3. Kiến trúc hệ thống đã chốt

```text
[Điện thoại (4G/5G hoặc bất kỳ Wi-Fi)]
        |
        | (Quét 1 mã QR cố định)
        v
[Web tĩnh: index.html (GitHub Pages)]
        |
        | (HTTP PUT / GET JSON)
        v
[Firebase Realtime Database (asia-southeast1) -> /dd60a.json]
        ^
        | (ESP32 poll mỗi 3s khi UART1 rảnh + đồng bộ giờ NTP UTC+7)
        |
[Borgwaldt DD60A] --(RS-232 2400 7-N-2)--> [MAX3232 #1] --> [ESP32 DevKit V1] --> [MAX3232 #2] --(RS-232 2400 7-N-1 / 9600 8-N-1)--> [Citizen CT-S310II]
```

### Cấu trúc JSON trên Firebase (`/dd60a.json` và `/dd60a_presets.json`)
* **`/dd60a.json`** (cấu hình lần đo hiện tại gửi xuống ESP32):
```json
{
  "sample": "Mau A1",
  "name": "Tuan",
  "type": "Dieu",
  "subType": "King Size",
  "circ": 24.5,
  "isManualCirc": false,
  "dec": 2,
  "testPrint": false
}
```
* **`/dd60a_presets.json`** (danh sách Phân loại & Chu vi mặc định, có thể thêm/sửa/xóa trực tiếp trên Web):
```json
[
  { "type": "Dieu", "name": "King Size", "circ": 24.5 },
  { "type": "Dieu", "name": "Demi Slim", "circ": 21.0 },
  { "type": "Dieu", "name": "Super Slim", "circ": 17.0 },
  { "type": "Dau loc", "name": "Dau loc Mono", "circ": 24.5 },
  { "type": "Dau loc", "name": "Dau loc Than", "circ": 24.5 }
]
```
*(Nếu người dùng sửa tay ô Chu vi trên Web cho một lần đo cụ thể, `isManualCirc` tự động chuyển thành `true` và phiếu in sẽ hiển thị là `Nhap tay (XX.XX mm)`).*

---

## 4. Danh sách linh kiện & Sơ đồ đấu dây (BOM & Wiring)

### Linh kiện cần dùng
* **1x ESP32 DevKit V1** (bản 30 hoặc 38 chân).
* **2x Module chuyển đổi RS232 sang TTL dùng chip MAX3232** *(Bắt buộc dùng chip MAX3232 chạy nguồn 3.3V; KHÔNG dùng MAX232 5V)*.
* **1x Bộ dây Dupont Cái–Cái (Female–Female)**, chuẩn 2.54mm, dài 10–20cm (dùng 8 sợi).
* **1x Nguồn 5V USB** (sạc điện thoại) cấp nguồn cho ESP32.
* **Cáp RS-232:** Dây DB9 nối DD60A vào Module #1; dây 9-pin ra 25-pin nguyên bản nối Module #2 sang Citizen.

### Bảng nối chân (ESP32 <-> 2x MAX3232)
| Chân trên MAX3232 | Module #1 (Nối DD60A - Serial1) | Module #2 (Nối Citizen - Serial2) |
| :--- | :--- | :--- |
| **VCC** | `3V3` trên ESP32 | `3V3` trên ESP32 |
| **GND** | `GND` trên ESP32 | `GND` trên ESP32 |
| **TXD** (TTL) | `GPIO 16` (RX1) | `GPIO 18` (RX2) |
| **RXD** (TTL) | `GPIO 17` (TX1) | `GPIO 19` (TX2) |

*(Lưu ý khi test: Một số module MAX3232 giá rẻ in ngược ký hiệu TXD/RXD phía TTL. Nếu không nhận/gửi được tín hiệu, đảo cặp chân 16/17 hoặc 18/19).*

---

## 5. Mã nguồn hiện tại (đã tách thành file riêng trong thư mục dự án)

* **Web App cấu hình:** [index.html](file:///d:/DD%2060%20A/index.html)
  * Nhập `SAMPLE` (tên mẫu), `NAME` (người đo) — tự động loại bỏ dấu tiếng Việt (`removeAccents()`) trước khi gửi lên Firebase.
  * Chọn `Loại` (`Dieu` / `Dau loc`) -> `Phân loại` (chứa chu vi mặc định), mặc định `-- Để trống (Không tính %) --` nếu chưa chọn, kèm mục **Quản lý phân loại & chu vi** lưu trên Firebase (`/dd60a_presets.json`).
  * Cho phép chỉnh tay `Chu vi (mm)` cho từng lần đo (khi chỉnh tay khác phân loại, phiếu in ghi rõ là `Nhap tay`).
* **Firmware ESP32:** [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino) *(Thư viện cần cài: `ArduinoJson` v7+)*.
  * Giao tiếp với Citizen CT-S310II mặc định `2400, SERIAL_7N1, X-ON/X-OFF` (có thể đổi đủ 8 mức Baud `1200`–`115200`, `7 bit / 8 bit`, `Parity`, `Handshaking` trên `http://192.168.4.1`), in 100% ASCII dùng Font B thu hẹp giãn dòng và in 2 cột để tiết kiệm giấy khổ 58mm.
  * Gom toàn bộ mẫu đo vào bộ đệm (`MAX_SAMPLES = 150`) và chỉ xuất in toàn bộ phiếu + cắt giấy khi gặp gói `FF05` (kèm timeout dự phòng `800 ms`); tự động xét STT mẫu (`sampleNum <= lastSampleNum` hoặc `0000` mới) để đẩy in phiếu cũ kèm cảnh báo `! CANH BAO: Mau da do chua duoc in truoc !`.
  * Nhận đồng thời cấu hình từ Firebase (`circ`, `sample`, `name`, `type`, `subType`, `isManualCirc`, `dec`, `testPrint`) và gói tin RS-232 từ DD60A.
  * Khi `circ > 0`, quy đổi `% = (giaTriDo / (chuVi / PI)) * 100`, làm tròn chuẩn toán học cả `mm` và `%` theo `dec`; khi `circ` là `null` (`<= 0`), chỉ in giá trị gốc `mm` và `V.C.%`.

---

## 6. Công việc cần làm tiếp theo trên Antigravity (Next Steps)

1. **Thiết lập hạ tầng Cloud:**
   * Tạo Firebase Project + Realtime Database (region `asia-southeast1`).
   * Cấu hình **Database Rules** cho nhánh `/dd60a` để chạy lâu dài không bị hết hạn 30 ngày như Test Mode.
   * Deploy `index.html` lên GitHub Pages và tạo mã QR cố định.
2. **Kiến trúc Firmware ESP32 (đã tích hợp sẵn trong `dd60a_bridge.ino`):**
   * **Lưu cấu hình & Wi-Fi vào NVS (`Preferences`):** Khi mất mạng Wi-Fi, ESP32 vẫn dùng cấu hình gần nhất để in bình thường (không treo ở `setup()`, `NGAY` hiển thị `--/--/---- --:--` nếu chưa có NTP).
   * **Captive Portal luôn bật (AP+STA):** Phát song song Wi-Fi `DD60A-Setup` (mật khẩu `12345678`, `http://192.168.4.1`) để đổi Wi-Fi hoặc chỉnh cấu hình cổng máy in Citizen (`Baud 1200–115200`, `7/8 bit`, `Parity`, `X-ON/X-OFF`) bất cứ lúc nào không cần nạp lại code.
   * **Tối ưu đa nhân FreeRTOS:** Poll Firebase & tự kết nối lại Wi-Fi (non-blocking) chạy ở Core 0; Core 1 chuyên tâm hứng RS-232 từ DD60A, in ra Citizen và phục vụ Web Server AP.
3. **Lắp ráp & Kiểm thử thực tế:**
   * Mua linh kiện, đấu dây Dupont theo bảng mục 4, nạp firmware và test toàn luồng `Web (4G) -> Firebase -> ESP32 -> DD60A -> Citizen CT-S310II`.