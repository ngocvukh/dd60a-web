# HẠNG MỤC 4: KIỂM THỬ TOÀN LUỒNG & XỬ LÝ SỰ CỐ

> **Mục tiêu:** Kiểm tra chạy thử từ Web -> Firebase -> ESP32 -> Citizen, sau đó test đo thật trên Borgwaldt DD60A và xử lý các lỗi thường gặp.

---

## Phần 1: Test luồng Web -> Firebase -> ESP32 -> Citizen (chưa cần bật DD60A)

1. Cấp nguồn 5V cho ESP32 và bật nguồn máy in Citizen CT-S310II.
2. Dùng điện thoại (4G hoặc Wi-Fi) quét mã QR để mở Web cấu hình.
3. Nhập thử:
   - **Tên mẫu:** `Mẫu A1`
   - **Người đo:** `Tuấn`
   - **Loại:** `Điếu` -> **Phân loại:** `King Size (24.50 mm)`
4. Bấm nút **LƯU & IN THỬ**.
5. Chờ tối đa 3–5 giây: máy in Citizen phải tự động in ra phiếu thử nghiệm (tiếng Việt không dấu ASCII), chia 2 cột gọn và tự cắt giấy.

---

## Phần 2: Test đo thật trên Borgwaldt DD60A

1. Bật máy **Borgwaldt DD60A**, cắm cáp RS-232 vào Module #1.
2. Trên Web, chọn đúng loại điếu/đầu lọc cần đo (hoặc gõ tay chu vi mới) rồi bấm **LƯU CẤU HÌNH**.
3. Tiến hành đo một loạt thử (1 mẫu hoặc nhiều mẫu >5 điếu) trên DD60A và bấm xuất/kết thúc loạt đo:
   - ESP32 sẽ gom toàn bộ các mẫu (`0001`–`0150`) và chờ đến khi nhận được gói tin kết thúc **`FF05`** mới tiến hành in toàn bộ phiếu ra cùng lúc (kèm timeout dự phòng `6000 ms` sau `FF01`–`FF04` nếu máy chỉ đo 1 mẫu và không gửi `FF05`; lưu ý thực tế DD60A vừa in kim vừa gửi `FF01`–`FF05` cách nhau `2.1s – 3.3s` mỗi gói).
   - *(Nếu người đo chưa bấm kết thúc trên DD60A mà sau đó lại đo đè lại STT mẫu cũ hoặc bắt đầu loạt mới, ESP32 sẽ tự động đẩy in phiếu cũ trước kèm dòng cảnh báo `! CANH BAO: Mau da do chua duoc in truoc !` rồi mới gom mẫu mới).*
4. Kiểm tra phiếu in ra từ Citizen:
   - Đủ phần đầu (`MAU`, `LOAI`, `NGAY | DO`).
   - Từng mẫu `01`, `02`, `03`... in 2 cột dạng `01:5.79|74.24%` (nếu Web để trống chu vi / `null` thì chỉ in `01:5.79mm`).
   - Đủ phần thống kê cuối phiếu (`MAX`, `MIN`, `MEAN`, `STD`, `V.C.`) và tự động cắt giấy.

---

## Phần 3: Bảng xử lý sự cố nhanh (Troubleshooting)

| Hiện tượng | Nguyên nhân thường gặp | Cách xử lý |
| :--- | :--- | :--- |
| Bấm **LƯU & IN THỬ** trên Web nhưng máy in không chạy | ESP32 chưa kết nối Wi-Fi (hoặc Wi-Fi 5GHz), sai link `FIREBASE_URL`, hoặc đảo ngược dây `18/19` | Mở Serial Monitor (`115200`) kiểm tra kết nối; kiểm tra link `/dd60a.json`; thử đảo cặp dây `GPIO 18` và `GPIO 19` trên Module #2 |
| Máy in ra chữ loằng ngoằng / ký tự lạ hoặc không phản hồi | `Baud rate`, `Data bits`, `Parity` hoặc `Handshaking` trên ESP32 chưa khớp với máy in Citizen | Kết nối Wi-Fi `DD60A-Setup` (`192.168.4.1`) và chọn lại đúng `Baud rate` (`1200`–`115200`), `Data bits` (`7`/`8`), `Parity` (`None`/`Even`/`Odd`) và `Handshaking` (`X-ON/X-OFF`/`None`) theo tờ Self-Test của máy in |
| Bấm in thử chạy bình thường nhưng đo trên DD60A không thấy in | Ngược chân `2 ↔ 3` trên cáp DB9, thiếu chập bắt tay `4 – 6 – 7` (`DSR/DTR/CTS`), hoặc ngược dây TTL `16/17` | Kiểm tra cáp từ `DD60A (25-pin)` sang `MAX3232 #1 (9-pin)` phải thông `DB25(2) ↔ DB9(3)`, `DB25(7) ↔ DB9(5)` và **chập 3 chân `4 – 6 – 7`** trên đầu 9-pin (`DB25 chân 5, 6, 20` thông nhau); mở Serial Monitor (`115200`) kiểm tra log `[DD60A RAW]` |
| Web báo `Lỗi kết nối mạng!` khi bấm Lưu | Chưa mở Rules trên Firebase hoặc sai `FIREBASE_BASE` trong `index.html` | Kiểm tra lại Hạng mục 1 (mục Rules) và đảm bảo `FIREBASE_BASE` không có `/dd60a.json` ở cuối |

---

**Mục lục:** [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md)
