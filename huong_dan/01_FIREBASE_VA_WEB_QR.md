# HẠNG MỤC 1: THIẾT LẬP FIREBASE, WEB CẤU HÌNH & MÃ QR

> **Mục tiêu:** Tạo cơ sở dữ liệu Firebase Realtime Database chạy lâu dài, gắn link vào Web [index.html](file:///d:/DD%2060%20A/index.html), đưa lên Internet (hỗ trợ 4G/5G) và tạo mã QR cố định dán tại vị trí đo.

---

## Phần 1: Thiết lập Firebase Realtime Database

1. Truy cập [Firebase Console](https://console.firebase.google.com/) và đăng nhập tài khoản Google.
2. Bấm **Create a project** (hoặc **Add project**) -> đặt tên dự án (VD: `dd60a-printer`) -> hoàn tất tạo project.
3. Ở menu bên trái, chọn **Build -> Realtime Database -> Create Database**.
4. Chọn vị trí máy chủ (Database location): **`Singapore (asia-southeast1)`** -> bấm **Next**.
5. Chọn **Start in test mode** -> bấm **Enable**.
6. Sau khi tạo xong, copy đường dẫn URL ở đầu bảng dữ liệu (có dạng: `https://TEN-PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app`).
7. Chuyển sang tab **Rules** (bên cạnh tab Data), thay toàn bộ nội dung bằng đoạn sau rồi bấm **Publish** (để không bị hết hạn sau 30 ngày như Test Mode):
   ```json
   {
     "rules": {
       "dd60a": {
         ".read": true,
         ".write": true
       },
       "dd60a_presets": {
         ".read": true,
         ".write": true
       }
     }
   }
   ```

---

## Phần 2: Cấu hình Web (`index.html`), Deploy & Tạo mã QR

### 2.1. Gắn link Firebase vào Web
1. Mở file [index.html](file:///d:/DD%2060%20A/index.html).
2. Tìm dòng `const FIREBASE_BASE = ...` (khoảng dòng 97) và thay bằng URL Firebase vừa copy ở Phần 1 (**không** có dấu `/` hay `/dd60a.json` ở cuối):
   ```javascript
   const FIREBASE_BASE = "https://TEN-PROJECT-default-rtdb.asia-southeast1.firebasedatabase.app";
   ```
3. Lưu file lại.

### 2.2. Đưa Web lên mạng bằng GitHub Pages (miễn phí, chạy qua 4G/5G)
1. Đăng nhập [GitHub](https://github.com/) và bấm dấu **`+`** góc trên bên phải -> **New repository**.
2. Đặt tên repository (VD: `dd60a-web`), chọn chế độ **Public** -> bấm **Create repository**.
3. Trên trang repo vừa tạo, bấm **uploading an existing file** (hoặc **Add file -> Upload files**).
4. Kéo-thả file `index.html` (đã sửa `FIREBASE_BASE`) vào -> bấm **Commit changes** để lưu lên nhánh `main`.
5. Vào tab **Settings** của repository -> ở menu bên trái chọn **Pages**.
6. Ở mục **Build and deployment**:
   - **Source:** chọn **Deploy from a branch**.
   - **Branch:** chọn **`main`** và thư mục **`/ (root)`** -> bấm **Save**.
7. Chờ khoảng 1–2 phút rồi tải lại trang **Settings -> Pages**, bạn sẽ thấy đường link HTTPS cố định ở đầu trang (dạng `https://ten-tai-khoan.github.io/dd60a-web/`).
   *(Sau này nếu muốn sửa giao diện Web, chỉ cần upload đè file `index.html` mới lên repo là link tự cập nhật, không đổi mã QR).*

### 2.3. Tạo mã QR cố định
1. Dùng bất kỳ trang tạo QR nào (hoặc chức năng tạo QR trên trình duyệt Chrome/Edge) dán link Web vừa có vào để tạo **1 mã QR duy nhất**.
2. In mã QR này ra và dán cố định lên thân máy Borgwaldt DD60A hoặc bàn thao tác.

---

**Bước tiếp theo:** [02_NAP_CODE_VA_CAU_HINH_ESP32.md](file:///d:/DD%2060%20A/huong_dan/02_NAP_CODE_VA_CAU_HINH_ESP32.md) | **Mục lục:** [HUONG_DAN_TRIEN_KHAI.md](file:///d:/DD%2060%20A/HUONG_DAN_TRIEN_KHAI.md)
