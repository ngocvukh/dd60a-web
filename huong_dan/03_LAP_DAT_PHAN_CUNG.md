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
- **1x Màn hình OLED 0.91 inch I2C 4 chân (SSD1306 128x32, tùy chọn)** để hiển thị trực tiếp loại điếu, phân loại và kích cỡ (chu vi).
- **1x LED Đỏ + 1x LED Xanh lá + 2x Điện trở `220Ω`–`330Ω`** (báo trạng thái mất/có Wi-Fi).
- **Dây Dupont Cái–Cái (Female–Female)** chuẩn 2.54mm (8 sợi cho 2x MAX3232 + 4 sợi cho OLED + dây nối LED).
- **1x Nguồn 5V USB** (củ sạc điện thoại ổn định) cấp cho ESP32.
- **Cáp RS-232:**
  - **Đầu vào (`Borgwaldt DD60A cổng 35 (25-pin)` -> `Module MAX3232 #1 (9-pin)`):**
    - Nếu dùng cáp `25-pin sang 9-pin` nguyên bản ghép nối tiếp cáp `9-pin sang 9-pin`, tại đầu `9-pin` của cáp `25-9` cần **đảo chéo chân `2 ↔ 3`** và **hàn nối chập 3 chân `4 – 6 – 7` với nhau** (để cấp vòng tín hiệu bắt tay `DTR 20 -> DSR 6 & CTS 5` cho DD60A).
    - Đo thông mạch xuyên suốt từ đầu `25-pin` (cắm DD60A) tới đầu `9-pin` cuối cùng (cắm `MAX3232 #1`) phải đạt:
      - `DB25 Chân 2 (TXD)` ↔ `DB9 Chân 3 (RXD MAX3232 #1)`
      - `DB25 Chân 3 (RXD)` ↔ `DB9 Chân 2 (TXD MAX3232 #1)`
      - `DB25 Chân 7 (GND)` ↔ `DB9 Chân 5 (GND MAX3232 #1)`
      - `DB25 Chân 5 (CTS)` ↔ `DB25 Chân 6 (DSR)` ↔ `DB25 Chân 20 (DTR)` *(thông qua mối chập `4 – 6 – 7` trên đầu 9-pin)*
    - Xem hình minh họa chi tiết tại [so_do_dau_day_25_9pin.png](../so_do_dau_day_25_9pin.png).
  - **Đầu ra (`Module MAX3232 #2 (9-pin)` -> `Citizen CT-S310II (9-pin)`):** Dùng cáp `9-pin` thẳng (`2–2`, `3–3`, `5–5`).

### 2.2. Sơ đồ đấu dây (ESP32 <-> 2x MAX3232)
*(Ngắt toàn bộ nguồn điện trước khi cắm dây Dupont)*

| Chân trên MAX3232 (phía TTL) | Module #1 (Nối DD60A - Serial1) | Module #2 (Nối Citizen - Serial2) |
| :--- | :--- | :--- |
| **VCC** | `3V3` trên ESP32 | `3V3` trên ESP32 |
| **GND** | `GND` trên ESP32 | `GND` trên ESP32 |
| **TXD** | `GPIO 17` (TX1) | `GPIO 19` (TX2) |
| **RXD** | `GPIO 16` (RX1) | `GPIO 18` (RX2) |

> **Lưu ý quan trọng:** Đã cập nhật lại cả Module #1 và Module #2 theo thực tế kiểm thử (`RXD` trên mạch MAX3232 nối `GPIO 16 (RX1)` / `GPIO 18 (RX2)`, `TXD` nối `GPIO 17 (TX1)` / `GPIO 19 (TX2)` do ký hiệu in ngược). Nếu thay module MAX3232 mới khác lô mà không nhận/không in được, chỉ cần đảo cặp dây `16 <-> 17` (cho Module #1) hoặc `18 <-> 19` (cho Module #2).

### 2.3. Sơ đồ đấu dây Màn hình OLED 4 chân (ESP32 <-> OLED I2C)

| Chân trên OLED | Chân trên ESP32 DevKit V1 |
| :--- | :--- |
| **GND** | `GND` |
| **VCC** | `3V3` |
| **SCL** *(hoặc **SCK**)* | `GPIO 22` |
| **SDA** | `GPIO 21` |

### 2.4. Sơ đồ đấu dây 2 đèn LED báo Wi-Fi

| Đèn LED | Chân Dương (`+`, chân dài) | Chân Âm (`-`, chân ngắn) | Ý nghĩa |
| :--- | :--- | :--- | :--- |
| **LED Đỏ** | Qua điện trở `220Ω`–`330Ω` -> **`GPIO 25`** | `GND` | Sáng khi **mất kết nối Wi-Fi** |
| **LED Xanh** | Qua điện trở `220Ω`–`330Ω` -> **`GPIO 26`** | `GND` | Sáng khi **đã kết nối Wi-Fi** |

---

**Bước tiếp theo:** [04_KIEM_THU_VA_XU_LY_SU_CO.md](file:///d:/DD%2060%20A/huong_dan/04_KIEM_THU_VA_XU_CO.md) | **Mục lục:** [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md)
