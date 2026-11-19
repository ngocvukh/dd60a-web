# HƯỚNG DẪN TRIỂN KHAI CHI TIẾT: CẦU NỐI IN NHIỆT DD60A -> CITIZEN CT-S310II

Tài liệu hướng dẫn được tách thành **4 hạng mục riêng biệt** theo đúng trình tự thực hiện để tiện theo dõi và tra cứu:

| STT | Hạng mục | Nội dung chính | Tài liệu |
| :---: | :--- | :--- | :--- |
| **1** | **Firebase, Web & Mã QR** | Tạo Realtime Database, cấu hình Rules lâu dài, gắn link vào [index.html](file:///d:/DD%2060%20A/index.html), deploy GitHub Pages và tạo mã QR | [01_FIREBASE_VA_WEB_QR.md](file:///d:/DD%2060%20A/huong_dan/01_FIREBASE_VA_WEB_QR.md) |
| **2** | **Nạp Code & Cấu hình ESP32** | Cài Arduino IDE + thư viện `ArduinoJson`, nạp [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino) và dùng Captive Portal `DD60A-Setup` (`192.168.4.1`) đổi Wi-Fi / chỉnh 8 mức Baud, 7/8 bit, Parity, X-ON/X-OFF | [02_NAP_CODE_VA_CAU_HINH_ESP32.md](file:///d:/DD%2060%20A/huong_dan/02_NAP_CODE_VA_CAU_HINH_ESP32.md) |
| **3** | **Lắp đặt Phần cứng** | Kiểm tra máy in Citizen (không cần chỉnh lại Memory Switch), danh sách linh kiện và bảng đấu dây ESP32 <-> 2x MAX3232 | [03_LAP_DAT_PHAN_CUNG.md](file:///d:/DD%2060%20A/huong_dan/03_LAP_DAT_PHAN_CUNG.md) |
| **4** | **Kiểm thử & Xử lý sự cố** | Test luồng Web -> ESP32 -> Citizen, test đo thật trên DD60A (cơ chế gom mẫu tới `FF05` & cảnh báo STT đè mẫu cũ), bảng Troubleshooting | [04_KIEM_THU_VA_XU_LY_SU_CO.md](file:///d:/DD%2060%20A/huong_dan/04_KIEM_THU_VA_XU_LY_SU_CO.md) |

---

### Các file mã nguồn & tài liệu liên quan
- **Web cấu hình qua QR:** [index.html](file:///d:/DD%2060%20A/index.html)
- **Firmware ESP32:** [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino)
- **Xem trước trang cài đặt ESP32 (`192.168.4.1`):** [esp32_wifi_setup_preview.html](file:///d:/DD%2060%20A/esp32_wifi_setup_preview.html)
- **Hồ sơ kỹ thuật & biên bản bàn giao:** [Phien lam viec Gemini.md](file:///d:/DD%2060%20A/Phien%20lam%20viec%20Gemini.md)
