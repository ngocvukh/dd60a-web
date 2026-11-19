# HẠNG MỤC 3: KIỂM TRA MÁY IN & LẮP ĐẶT PHẦN CỨNG

> **Mục tiêu:** Đấu nối hoàn chỉnh ESP32 DevKit V1 với 2 module MAX3232, máy đo Borgwaldt DD60A và máy in Citizen CT-S310II (không cần chỉnh lại Memory Switch trên máy in).

---

## Phần 1: Kiểm tra cấu hình máy in Citizen CT-S310II (Không cần chỉnh lại máy in)

Vì ESP32 mặc định đang chạy **`2400 bps | 7-N-1 | X-ON/X-OFF`** khớp sẵn với tờ Self-Test hiện tại của máy in Citizen, bạn **cắm vào là chạy ngay, không cần người sử dụng biết cách chỉnh Memory Switch trên máy in**.

- **Chỉ khi nào thay đổi thông số cổng Serial máy in (`Baud`, `7/8 bit`, `Parity`, `Handshaking`):**
  - Không cần chạm vào máy in hay nạp lại code — chỉ cần kết nối vào Wi-Fi **`DD60A-Setup`** -> vào `http://192.168.4.1` và chọn lại các thông số khớp đúng với tờ Self-Test của máy in đang dùng (nhấn giữ nút `FEED` + bật nguồn máy in để xem tờ Self-Test).

---

## Phần 2: Danh sách linh kiện & Sơ đồ đấu dây

### 2.1. Danh sách linh kiện
- **1x ESP32 DevKit V1** (30 hoặc 38 chân).
- **2x Module chuyển đổi RS-232 sang TTL dùng chip MAX3232** *(bắt buộc dùng bản hỗ trợ 3.3V; **KHÔNG** dùng MAX232 5V vì có thể hỏng chân GPIO của ESP32)*.
- **8 sợi dây Dupont Cái–Cái (Female–Female)** chuẩn 2.54mm.
- **1x Nguồn 5V USB** (củ sạc điện thoại ổn định) cấp cho ESP32.
- **Cáp RS-232:**
  - Dây nối cổng số 35 trên **Borgwaldt DD60A** vào cổng DB9 của **Module MAX3232 #1**.
  - Dây 9-pin ra 25-pin nguyên bản nối cổng DB9 của **Module MAX3232 #2** vào **Citizen CT-S310II**.

### 2.2. Sơ đồ đấu dây (ESP32 <-> 2x MAX3232)
*(Ngắt toàn bộ nguồn điện trước khi cắm dây Dupont)*

| Chân trên MAX3232 (phía TTL) | Module #1 (Nối DD60A - Serial1) | Module #2 (Nối Citizen - Serial2) |
| :--- | :--- | :--- |
| **VCC** | `3V3` trên ESP32 | `3V3` trên ESP32 |
| **GND** | `GND` trên ESP32 | `GND` trên ESP32 |
| **TXD** | `GPIO 16` (RX1) | `GPIO 18` (RX2) |
| **RXD** | `GPIO 17` (TX1) | `GPIO 19` (TX2) |

> **Lưu ý quan trọng:** Một số mạch MAX3232 giá rẻ in ngược ký hiệu `TXD`/`RXD` ở phía hàng rào chân TTL. Nếu đã nối đúng bảng trên mà không nhận/không in được, chỉ cần đảo cặp dây `16 <-> 17` (cho Module #1) hoặc `18 <-> 19` (cho Module #2).

---

**Bước tiếp theo:** [04_KIEM_THU_VA_XU_LY_SU_CO.md](file:///d:/DD%2060%20A/huong_dan/04_KIEM_THU_VA_XU_CO.md) | **Mục lục:** [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md)
