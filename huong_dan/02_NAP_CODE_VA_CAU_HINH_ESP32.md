# HẠNG MỤC 2: NẠP CODE & CẤU HÌNH ESP32 (WI-FI + CỔNG MÁY IN)

> **Mục tiêu:** Cài đặt Arduino IDE, nạp Firmware [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino) cho ESP32 DevKit V1 và sử dụng trang nội bộ `DD60A-Setup` (`192.168.4.1`) để đổi Wi-Fi hoặc cấu hình máy in Citizen mà không cần nạp lại code.

---

## Phần 1: Cài Arduino IDE & Thư viện

1. Tải và cài đặt **Arduino IDE 2.x**.
2. Nếu máy tính chưa nhận cổng COM khi cắm ESP32 qua cáp USB: cài driver **CP2102** (Silicon Labs) hoặc **CH340** tùy chip nạp trên board ESP32 của bạn.
3. Cài board ESP32:
   - Vào **File -> Preferences**, ở ô *Additional boards manager URLs* dán thêm:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Vào **Tools -> Board -> Boards Manager**, tìm `esp32` (của *Espressif Systems*) và bấm **Install**.
4. Cài thư viện **ArduinoJson**:
   - Vào **Tools -> Manage Libraries**, tìm `ArduinoJson` (tác giả *Benoit Blanchon*, bản **v7.x trở lên**) và bấm **Install**.

---

## Phần 2: Cấu hình & Nạp Firmware cho ESP32

1. Mở file [dd60a_bridge.ino](file:///d:/DD%2060%20A/dd60a_bridge/dd60a_bridge.ino) bằng Arduino IDE.
2. Sửa dòng `FIREBASE_URL` ở đầu file (có thể sửa thêm `DEFAULT_WIFI_SSID` và `DEFAULT_WIFI_PASS` nếu muốn, hoặc để nguyên và cấu hình Wi-Fi qua điện thoại ở Phần 3):
   ```cpp
   const char* DEFAULT_WIFI_SSID = "TEN_WIFI_CO_INTERNET";
   const char* DEFAULT_WIFI_PASS = "MAT_KHAU_WIFI";
   const char* FIREBASE_URL      = "https://TEN-PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app/dd60a.json";
   ```
   *(Lưu ý: `FIREBASE_URL` ở ESP32 **phải có** `/dd60a.json` ở cuối, và Wi-Fi phải là Wi-Fi 2.4GHz — ESP32 không kết nối được Wi-Fi 5GHz).*
3. Cắm ESP32 vào máy tính qua cáp USB **có truyền dữ liệu** (tránh cáp chỉ sạc).
4. Chọn **Tools -> Board -> esp32 -> DOIT ESP32 DEVKIT V1**, chọn đúng **Tools -> Port -> COMx**.
5. Bấm nút **Upload** (mũi tên sang phải). Nếu thấy màn hình dừng ở `Connecting........`, nhấn giữ nút **BOOT** trên board ESP32 khoảng 2 giây rồi thả ra.
6. Khi báo `Done uploading`, mở **Tools -> Serial Monitor** (chọn tốc độ `115200 baud`), nhấn nút **EN/RST** trên ESP32 và kiểm tra thấy dòng `He thong san sang! AP: DD60A-Setup (IP: 192.168.4.1)`.

---

## Phần 3: Đổi Wi-Fi & Cấu hình cổng máy in Citizen không cần nạp lại code (Captive Portal)

ESP32 luôn phát song song một mạng Wi-Fi nội bộ để bạn cài đặt bất cứ lúc nào:
1. Dùng điện thoại hoặc laptop kết nối vào Wi-Fi tên **`DD60A-Setup`** (mật khẩu: **`12345678`**).
2. Trang cấu hình sẽ tự bật lên (hoặc mở trình duyệt truy cập **`http://192.168.4.1`**).
3. **Mục Cấu hình Máy in Citizen:** Mặc định ESP32 đã đặt sẵn **`2400 bps, 7 bits, Parity: None, Handshaking: X-ON/X-OFF`** khớp với tờ Self-Test hiện tại của máy in Citizen nên bạn **không cần chỉnh gì thêm**. Nếu sau này đổi máy in khác (hoặc máy reset về `9600 bps, 8 bits`), bạn có thể chọn đủ cả **8 mức Baud (`1200`–`115200`)**, **Data bits (`7`/`8`)**, **Parity (`None`/`Even`/`Odd`)** và **Handshaking (`X-ON/X-OFF`/`None`)** rồi bấm **LƯU CẤU HÌNH MÁY IN** (áp dụng ngay không cần khởi động lại).
4. **Mục Cấu hình Wi-Fi ESP32:** Bấm **Quét mạng Wi-Fi xung quanh**, chọn tên Wi-Fi mới (2.4GHz), nhập mật khẩu -> bấm **LƯU WI-FI & KẾT NỐI LẠI**. ESP32 sẽ lưu vào bộ nhớ Flash (NVS) và tự kết nối Wi-Fi mới.
5. *(Nếu Wi-Fi mất mạng giữa chừng hoặc mất từ lúc bật máy, ESP32 vẫn gom đủ mẫu từ DD60A và in bình thường bằng cấu hình đã lưu trong Flash; dòng ngày giờ sẽ hiển thị `--/--/---- --:--` nếu chưa lấy được giờ NTP. Trường hợp chưa có thông tin chu vi từ Web gửi về (`circ` là `null`), ESP32 sẽ **không tự tính `%`** mà chỉ in giá trị gốc `mm` cùng `V.C.%` từ máy đo).*

---

**Bước tiếp theo:** [03_LAP_DAT_PHAN_CUNG.md](file:///d:/DD%2060%20A/huong_dan/03_LAP_DAT_PHAN_CUNG.md) | **Mục lục:** [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md)
