#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== 1. CAU HINH MAC DINH (DU PHONG KHI CHUA CAI QUA WEB AP) =====
const char* DEFAULT_WIFI_SSID = "TEN_WIFI_CO_INTERNET";
const char* DEFAULT_WIFI_PASS = "MAT_KHAU_WIFI";
const char* FIREBASE_URL      = "https://dd60a-244ce-default-rtdb.asia-southeast1.firebasedatabase.app/dd60a.json";

// ===== 2. CAU HINH WI-FI PHAT (CAPTIVE PORTAL) =====
const char* AP_SSID = "DD60A-Setup";
const char* AP_PASS = "12345678";

// ===== 3. CHAN KET NOI MAX3232 =====
#define DD60A_RX_PIN   16
#define DD60A_TX_PIN   17
#define PRINTER_RX_PIN 18
#define PRINTER_TX_PIN 19

// ===== 3.1. CHAN MAN HINH OLED 0.91 INCH I2C (SSD1306 128x32) =====
#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22
#define OLED_WIDTH   128
#define OLED_HEIGHT  32
#define OLED_ADDR    0x3C

// ===== 3.2. CHAN DEN LED BAO WI-FI =====
#define LED_RED_PIN   25  // Sang khi mat ket noi Wi-Fi
#define LED_GREEN_PIN 26  // Sang khi da ket noi Wi-Fi

// ===== 4. BO DEM GOM MAU DO (TOI DA 150 MAU THEO MANUAL DD60A) =====
#define MAX_SAMPLES 150

struct SampleItem {
  int    num;
  double valMm;
};

// ===== BIEN CAU HINH BAN IN (DONG BO GIUA CORE 0 VA CORE 1) =====
struct PrintConfig {
  String sample       = "";
  String name         = "";
  String type         = "";
  String subType      = "";
  double circ         = 0.0;   // <= 0 nghia la null (chua co chu vi -> khong tinh %)
  bool   isManualCirc = false;
  int    dec          = 2;
};

PrintConfig cfg;
SemaphoreHandle_t cfgMutex = NULL;

String wifiSsid = "";
String wifiPass = "";
int    printerBaud       = 2400; // 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
int    printerBits       = 7;    // 7 or 8
char   printerParity     = 'N';  // 'N' (None), 'E' (Even), 'O' (Odd)
char   printerHandshake  = 'X';  // 'X' (X-ON/X-OFF), 'N' (None)
bool   printerXoffPaused = false;
unsigned long xoffStartTime = 0;

volatile bool     pendingTestPrint       = false;
volatile bool     pendingClearAfterPrint = false;
volatile uint32_t clearConfigEpoch       = 0;
volatile bool     ntpConfigured          = false;
volatile bool     isScanningWifi         = false;

String packetBuffer    = "";
bool   receivingPacket = false;
String pendingSample   = "";
bool          rawDebugLineOpen = false;
unsigned long lastRawRxTime    = 0;

SampleItem batchSamples[MAX_SAMPLES];
int    batchSampleCount = 0;
bool   hasMax  = false, hasMin  = false, hasMean = false, hasStd  = false, hasVc   = false;
double statMaxMm = 0.0, statMinMm = 0.0, statMeanMm = 0.0, statStdMm = 0.0, statVcPct = 0.0;
unsigned long lastStatPacketTime = 0;

Preferences prefs;
WebServer   webServer(80);
DNSServer   dnsServer;
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
bool oledReady = false;
unsigned long lastOledCheck = 0;
String lastOledKey = "";

// ===== LUU / NAP NVS (FLASH) =====
void loadWifiFromNVS() {
  prefs.begin("wifi", true);
  wifiSsid = prefs.getString("ssid", DEFAULT_WIFI_SSID);
  wifiPass = prefs.getString("pass", DEFAULT_WIFI_PASS);
  prefs.end();
}

void saveWifiToNVS(const String& ssid, const String& pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

bool isValidBaud(int b) {
  return (b == 1200 || b == 2400 || b == 4800 || b == 9600 ||
          b == 19200 || b == 38400 || b == 57600 || b == 115200);
}

void loadPrinterConfigFromNVS() {
  prefs.begin("printer", true);
  printerBaud      = prefs.getInt("baud", 2400);
  printerBits      = prefs.getInt("bits", 7);
  String pStr      = prefs.getString("parity", "N");
  String hStr      = prefs.getString("hs", "X");
  prefs.end();
  if (!isValidBaud(printerBaud)) printerBaud = 2400;
  if (printerBits != 7 && printerBits != 8) printerBits = 7;
  printerParity    = (pStr.length() > 0 && (pStr[0] == 'E' || pStr[0] == 'O')) ? pStr[0] : 'N';
  printerHandshake = (hStr.length() > 0 && hStr[0] == 'N') ? 'N' : 'X';
}

void savePrinterConfigToNVS(int baud, int bits, char parity, char hs) {
  prefs.begin("printer", false);
  prefs.putInt("baud", baud);
  prefs.putInt("bits", bits);
  prefs.putString("parity", String(parity));
  prefs.putString("hs", String(hs));
  prefs.end();
}

void applyPrinterSerial() {
  Serial2.end();
  uint32_t cfgMode = SERIAL_7N1;
  if (printerBits == 7) {
    if (printerParity == 'E') cfgMode = SERIAL_7E1;
    else if (printerParity == 'O') cfgMode = SERIAL_7O1;
    else cfgMode = SERIAL_7N1;
  } else {
    if (printerParity == 'E') cfgMode = SERIAL_8E1;
    else if (printerParity == 'O') cfgMode = SERIAL_8O1;
    else cfgMode = SERIAL_8N1;
  }
  printerXoffPaused = false;
  Serial2.begin(printerBaud, cfgMode, PRINTER_RX_PIN, PRINTER_TX_PIN);
}

void checkPrinterFlowControl() {
  if (printerHandshake != 'X') return;
  while (Serial2.available()) {
    byte b = Serial2.read() & 0x7F;
    if (b == 0x13) { // XOFF
      printerXoffPaused = true;
      xoffStartTime = millis();
    } else if (b == 0x11) { // XON
      printerXoffPaused = false;
    }
  }
}

void printerPrintln(const String& line) {
  if (printerHandshake == 'X') {
    checkPrinterFlowControl();
    while (printerXoffPaused && (millis() - xoffStartTime < 2000)) {
      delay(5);
      checkPrinterFlowControl();
    }
    printerXoffPaused = false;
  }
  Serial2.println(line);
  Serial.println(line);
}

void loadConfigFromNVS() {
  PrintConfig tmp;
  prefs.begin("cfg", true);
  tmp.sample       = prefs.getString("sample", "");
  tmp.name         = prefs.getString("name", "");
  tmp.type         = prefs.getString("type", "");
  tmp.subType      = prefs.getString("subType", "");
  tmp.circ         = prefs.getDouble("circ", 0.0);
  tmp.isManualCirc = prefs.getBool("manual", false);
  tmp.dec          = prefs.getInt("dec", 2);
  prefs.end();

  xSemaphoreTake(cfgMutex, portMAX_DELAY);
  cfg = tmp;
  xSemaphoreGive(cfgMutex);
}

void saveConfigToNVS(const PrintConfig& c) {
  prefs.begin("cfg", false);
  prefs.putString("sample", c.sample);
  prefs.putString("name", c.name);
  prefs.putString("type", c.type);
  prefs.putString("subType", c.subType);
  prefs.putDouble("circ", c.circ);
  prefs.putBool("manual", c.isManualCirc);
  prefs.putInt("dec", c.dec);
  prefs.end();
}

PrintConfig getConfigSnapshot() {
  xSemaphoreTake(cfgMutex, portMAX_DELAY);
  PrintConfig copy = cfg;
  xSemaphoreGive(cfgMutex);
  return copy;
}

// ===== CAPTIVE PORTAL WEB SERVER (CHAY TREN CORE 1) =====
String htmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  return s;
}

void handlePortalRoot() {
  bool connected = (WiFi.status() == WL_CONNECTED);
  String statusHtml = connected
    ? "<span style='color:#137333;font-weight:bold;'>Đã kết nối (" + WiFi.localIP().toString() + ")</span>"
    : "<span style='color:#c5221f;font-weight:bold;'>Mất kết nối (Đang chạy chế độ in Offline)</span>";

  String optionsHtml = "";
  String scanMsgHtml = "";
  if (webServer.hasArg("scan") && !receivingPacket && !Serial1.available()) {
    isScanningWifi = true;
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect(false);
      delay(120);
    }
    int n = WiFi.scanNetworks(false, false, false, 300);
    if (n > 0) {
      int added = 0;
      for (int i = 0; i < n; i++) {
        String s = WiFi.SSID(i);
        if (s.length() == 0) continue;
        String sel = (s == wifiSsid) ? " selected" : "";
        optionsHtml += "<option value=\"" + htmlEscape(s) + "\"" + sel + ">" + htmlEscape(s) + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
        added++;
      }
      scanMsgHtml = "<div style='color:#137333;font-size:12px;margin-top:4px;'>Đã quét thấy " + String(added) + " mạng Wi-Fi 2.4GHz.</div>";
    } else {
      scanMsgHtml = "<div style='color:#c5221f;font-size:12px;margin-top:4px;'>Không tìm thấy mạng Wi-Fi 2.4GHz nào (mã: " + String(n) + "). Hãy thử bấm quét lại hoặc nhập tay SSID.</div>";
    }
    WiFi.scanDelete();
    isScanningWifi = false;
  }

  String sel1200   = (printerBaud == 1200)   ? " selected" : "";
  String sel2400   = (printerBaud == 2400)   ? " selected" : "";
  String sel4800   = (printerBaud == 4800)   ? " selected" : "";
  String sel9600   = (printerBaud == 9600)   ? " selected" : "";
  String sel19200  = (printerBaud == 19200)  ? " selected" : "";
  String sel38400  = (printerBaud == 38400)  ? " selected" : "";
  String sel57600  = (printerBaud == 57600)  ? " selected" : "";
  String sel115200 = (printerBaud == 115200) ? " selected" : "";
  String sel7bit   = (printerBits == 7)      ? " selected" : "";
  String sel8bit   = (printerBits == 8)      ? " selected" : "";
  String selParN   = (printerParity == 'N')  ? " selected" : "";
  String selParE   = (printerParity == 'E')  ? " selected" : "";
  String selParO   = (printerParity == 'O')  ? " selected" : "";
  String selHsX    = (printerHandshake == 'X') ? " selected" : "";
  String selHsN    = (printerHandshake == 'N') ? " selected" : "";
  String hsLabel   = (printerHandshake == 'X') ? "X-ON/X-OFF" : "None";

  String page = R"rawliteral(
<!DOCTYPE html><html lang="vi"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cài Đặt ESP32 DD60A</title>
<style>
  body{font-family:Arial,sans-serif;max-width:430px;margin:16px auto;padding:0 12px;background:#f4f6f8;color:#222;}
  .card{background:#fff;padding:18px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.1);margin-bottom:14px;}
  h2{margin-top:0;font-size:18px;color:#1a73e8;text-align:center;}
  label{display:block;margin-top:12px;font-weight:bold;font-size:14px;}
  input,select{width:100%;padding:10px;margin-top:4px;box-sizing:border-box;border:1px solid #ccc;border-radius:6px;font-size:15px;}
  button{width:100%;padding:12px;margin-top:14px;border:none;border-radius:6px;background:#1a73e8;color:#fff;font-size:15px;font-weight:bold;cursor:pointer;}
  .btn-green{background:#34a853;}
  .info{background:#f8f9fa;padding:10px;border-radius:6px;font-size:13px;line-height:1.6;}
</style></head><body>

<div class="card">
  <h2>TRẠNG THÁI HỆ THỐNG DD60A</h2>
  <div class="info">
    <div><b>Wi-Fi đang lưu:</b> )rawliteral" + htmlEscape(wifiSsid) + R"rawliteral(</div>
    <div><b>Trạng thái mạng:</b> )rawliteral" + statusHtml + R"rawliteral(</div>
    <div><b>Cổng máy in Citizen:</b> )rawliteral" + String(printerBaud) + " bps, " + String(printerBits) + "-" + String(printerParity) + "-1, " + hsLabel + R"rawliteral(</div>
  </div>
</div>

<div class="card">
  <h2>CẤU HÌNH MÁY IN CITIZEN</h2>
  <form method="POST" action="/save-printer">
    <label>Tốc độ truyền (Baud rate):</label>
    <select name="baud">
      <option value="1200")rawliteral" + sel1200 + R"rawliteral(>1200 bps</option>
      <option value="2400")rawliteral" + sel2400 + R"rawliteral(>2400 bps (Mặc định máy in hiện tại)</option>
      <option value="4800")rawliteral" + sel4800 + R"rawliteral(>4800 bps</option>
      <option value="9600")rawliteral" + sel9600 + R"rawliteral(>9600 bps (Chuẩn xuất xưởng Citizen)</option>
      <option value="19200")rawliteral" + sel19200 + R"rawliteral(>19200 bps</option>
      <option value="38400")rawliteral" + sel38400 + R"rawliteral(>38400 bps</option>
      <option value="57600")rawliteral" + sel57600 + R"rawliteral(>57600 bps</option>
      <option value="115200")rawliteral" + sel115200 + R"rawliteral(>115200 bps</option>
    </select>
    <label>Số bit dữ liệu (Data bits):</label>
    <select name="bits">
      <option value="7")rawliteral" + sel7bit + R"rawliteral(>7 bits</option>
      <option value="8")rawliteral" + sel8bit + R"rawliteral(>8 bits</option>
    </select>
    <label>Kiểm tra chẵn lẻ (Parity):</label>
    <select name="parity">
      <option value="N")rawliteral" + selParN + R"rawliteral(>None - Không (Mặc định)</option>
      <option value="E")rawliteral" + selParE + R"rawliteral(>Even - Chẵn</option>
      <option value="O")rawliteral" + selParO + R"rawliteral(>Odd - Lẻ</option>
    </select>
    <label>Bắt tay (Handshaking):</label>
    <select name="hs">
      <option value="X")rawliteral" + selHsX + R"rawliteral(>X-ON/X-OFF (Mặc định Citizen)</option>
      <option value="N")rawliteral" + selHsN + R"rawliteral(>None - Không dùng</option>
    </select>
    <button type="submit" class="btn-green">LƯU CẤU HÌNH MÁY IN</button>
  </form>
</div>

<div class="card">
  <h2>CẤU HÌNH WI-FI ESP32</h2>
  <form method="POST" action="/save">
    <label>Chọn mạng Wi-Fi (2.4GHz):</label>
    <a href="/?scan=1" style="display:inline-block;margin-top:8px;font-size:13px;color:#1a73e8;text-decoration:none;font-weight:bold;">🔄 Quét mạng Wi-Fi xung quanh</a>
    <select id="scan" onchange="document.getElementById('ssid').value=this.value">
      <option value="">-- Chọn từ danh sách quét --</option>)rawliteral" + optionsHtml + R"rawliteral(
    </select>)rawliteral" + scanMsgHtml + R"rawliteral(
    <label>Hoặc nhập tên Wi-Fi (SSID):</label>
    <input type="text" id="ssid" name="ssid" value=")rawliteral" + htmlEscape(wifiSsid) + R"rawliteral(" required>
    <label>Mật khẩu Wi-Fi:</label>
    <input type="password" name="pass" placeholder="Để trống nếu Wi-Fi không mật khẩu">
    <button type="submit">LƯU WI-FI &amp; KẾT NỐI LẠI</button>
  </form>
</div>

</body></html>)rawliteral";

  webServer.send(200, "text/html", page);
}

void handlePortalSavePrinter() {
  int baud   = webServer.arg("baud").toInt();
  int bits   = webServer.arg("bits").toInt();
  String pIn = webServer.arg("parity");
  String hIn = webServer.arg("hs");

  if (!isValidBaud(baud)) baud = 2400;
  if (bits != 7 && bits != 8) bits = 7;
  char parity = (pIn.length() > 0 && (pIn[0] == 'E' || pIn[0] == 'O')) ? pIn[0] : 'N';
  char hs     = (hIn.length() > 0 && hIn[0] == 'N') ? 'N' : 'X';

  printerBaud      = baud;
  printerBits      = bits;
  printerParity    = parity;
  printerHandshake = hs;
  savePrinterConfigToNVS(printerBaud, printerBits, printerParity, printerHandshake);
  applyPrinterSerial();

  String hsLabel = (printerHandshake == 'X') ? "X-ON/X-OFF" : "None";
  String html = "<!DOCTYPE html><html lang='vi'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
                "<body style='font-family:Arial;text-align:center;padding:30px;'>"
                "<h3 style='color:#137333;'>Đã cập nhật cổng máy in: " + String(printerBaud) + " bps, " + String(printerBits) + "-" + String(printerParity) + "-1, " + hsLabel + "</h3>"
                "<p>Áp dụng ngay lập tức (không cần khởi động lại ESP32).</p>"
                "<p><a href='/' style='color:#1a73e8;font-weight:bold;'>← Quay lại trang cấu hình</a></p></body></html>";
  webServer.send(200, "text/html", html);
}

void handlePortalSave() {
  String newSsid = webServer.arg("ssid");
  String newPass = webServer.arg("pass");
  newSsid.trim();

  if (newSsid.length() == 0) {
    webServer.send(400, "text/plain; charset=utf-8", "Ten Wi-Fi khong duoc de trong!");
    return;
  }

  saveWifiToNVS(newSsid, newPass);
  String html = "<!DOCTYPE html><html lang='vi'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'></head>"
                "<body style='font-family:Arial;text-align:center;padding:30px;'>"
                "<h3 style='color:#137333;'>Đã lưu Wi-Fi mới: " + htmlEscape(newSsid) + "</h3>"
                "<p>ESP32 đang khởi động lại để kết nối...</p></body></html>";
  webServer.send(200, "text/html", html);
  delay(800);
  ESP.restart();
}

void handlePortalNotFound() {
  if (webServer.uri() == "/favicon.ico") {
    webServer.send(204, "text/plain", "");
    return;
  }
  webServer.sendHeader("Location", "http://192.168.4.1/", true);
  webServer.send(302, "text/plain", "");
}

// ===== XU LY THOI GIAN & DINH DANG SO =====
String getCurrentDateTime() {
  struct tm timeinfo;
  // Timeout 10ms de khong bao gio chan luong in khi mat mang
  if (!getLocalTime(&timeinfo, 10)) return "--/--/---- --:--";
  if (timeinfo.tm_year + 1900 < 2024) return "--/--/---- --:--";
  char timeStr[25];
  strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M", &timeinfo);
  return String(timeStr);
}

double parseRawMm(String raw) {
  raw.trim();
  return raw.toDouble() / 10000.0;
}

String formatRounded(double val, int decimals) {
  if (decimals < 0) decimals = 0;
  if (decimals > 4) decimals = 4;
  double factor = pow(10.0, decimals);
  double rounded = round(val * factor) / factor;
  return String(rounded, decimals);
}

double mmToPercent(double valMm, double circ) {
  if (circ <= 0.0) return 0.0;
  double diameter = circ / M_PI;
  return (valMm / diameter) * 100.0;
}

// ===== IN AN ESC/POS (100% ASCII, CORE 1) =====
void initPrinterEscPos() {
  Serial2.write(0x1B); Serial2.write(0x40); // ESC @ : Khoi tao may in
  Serial2.write(0x1B); Serial2.write(0x4D); Serial2.write(0x01); // ESC M 1 : Font B (gon)
  Serial2.write(0x1B); Serial2.write(0x33); Serial2.write(0x14); // ESC 3 20 : Thu hep gian dong
}

void printHeader(const PrintConfig& c) {
  initPrinterEscPos();
  pendingSample = "";

  printerPrintln("MAU : " + c.sample);
  if (c.circ > 0.0) {
    if (c.isManualCirc) {
      printerPrintln("LOAI: " + c.type + " - Nhap tay (" + formatRounded(c.circ, 2) + "mm)");
    } else {
      printerPrintln("LOAI: " + c.type + " - " + c.subType + " (" + formatRounded(c.circ, 2) + "mm)");
    }
  } else {
    if (c.type.length() > 0) {
      printerPrintln("LOAI: " + c.type + " (null)");
    } else {
      printerPrintln("LOAI: null");
    }
  }
  printerPrintln("NGAY: " + getCurrentDateTime() + " | DO: " + c.name);
  printerPrintln("------------------------------------------");
}

void flushPendingSample() {
  if (pendingSample.length() > 0) {
    printerPrintln(pendingSample);
    pendingSample = "";
  }
}

String formatDualValue(double valMm, bool hasCirc, double circ, int dec) {
  if (hasCirc) {
    double valPct = mmToPercent(valMm, circ);
    return formatRounded(valMm, dec) + "|" + formatRounded(valPct, dec) + "%";
  }
  return formatRounded(valMm, dec) + "mm";
}

void appendSample2Col(int sampleNum, double valMm, bool hasCirc, double circ, int dec) {
  char numBuf[6];
  snprintf(numBuf, sizeof(numBuf), "%02d", sampleNum);
  String entry = String(numBuf) + ":" + formatDualValue(valMm, hasCirc, circ, dec);

  if (pendingSample.length() == 0) {
    pendingSample = entry;
  } else {
    while (pendingSample.length() < 20) pendingSample += " ";
    printerPrintln(pendingSample + " " + entry);
    pendingSample = "";
  }
}

void cutPaper() {
  flushPendingSample();
  printerPrintln("------------------------------------------");
  printerPrintln("");
  Serial2.write(0x1D); Serial2.write(0x56); Serial2.write(0x42); Serial2.write(0x00);
}

void resetBatchBuffer() {
  batchSampleCount = 0;
  hasMax = hasMin = hasMean = hasStd = hasVc = false;
  lastStatPacketTime = 0;
}

void updateOledDisplay(bool force);

void clearBatchConfigAfterPrint() {
  PrintConfig cleared;
  xSemaphoreTake(cfgMutex, portMAX_DELAY);
  cfg.sample       = "";
  cfg.name         = "";
  cfg.type         = "";
  cfg.subType      = "";
  cfg.circ         = 0.0;
  cfg.isManualCirc = false;
  // Giu nguyen cfg.dec
  cleared = cfg;
  clearConfigEpoch++;
  pendingClearAfterPrint = true;
  xSemaphoreGive(cfgMutex);

  saveConfigToNVS(cleared);
  updateOledDisplay(true);
  Serial.println("[DD60A] Da reset ten mau, nguoi do, phan loai & chu vi sau khi in.");
}

void printFullBatch(bool isUnfinishedWarning) {
  if (batchSampleCount == 0 && !hasMax && !hasMin && !hasMean && !hasStd && !hasVc) {
    resetBatchBuffer();
    return;
  }

  PrintConfig c = getConfigSnapshot();
  bool hasCirc = (c.circ > 0.0);

  printHeader(c);

  if (isUnfinishedWarning) {
    printerPrintln("! CANH BAO: Mau da do chua duoc in truoc !");
    printerPrintln("------------------------------------------");
  }

  for (int i = 0; i < batchSampleCount; i++) {
    appendSample2Col(batchSamples[i].num, batchSamples[i].valMm, hasCirc, c.circ, c.dec);
  }
  flushPendingSample();

  if (hasMax || hasMin || hasMean || hasStd || hasVc) {
    printerPrintln("------------------------------------------");
    if (hasMax)  printerPrintln("MAX : " + formatDualValue(statMaxMm, hasCirc, c.circ, c.dec));
    if (hasMin)  printerPrintln("MIN : " + formatDualValue(statMinMm, hasCirc, c.circ, c.dec));
    if (hasMean) printerPrintln("MEAN: " + formatDualValue(statMeanMm, hasCirc, c.circ, c.dec));
    if (hasStd)  printerPrintln("STD : " + formatDualValue(statStdMm, hasCirc, c.circ, 4));
    if (hasVc)   printerPrintln("COEFFICIENT OF VARIATION (V.C.): " + formatRounded(statVcPct, 2) + "%");
  }

  cutPaper();
  resetBatchBuffer();
  if (!isUnfinishedWarning) {
    clearBatchConfigAfterPrint();
  }
}

void processPacket(String data) {
  data.trim();
  if (rawDebugLineOpen) {
    Serial.println();
    rawDebugLineOpen = false;
  }
  Serial.printf("[DD60A PKT] '%s' (len=%d)\n", data.c_str(), data.length());
  if (data.length() < 11) return;

  String code   = data.substring(0, 4);
  String valRaw = data.substring(5);

  if (code == "0000") {
    if (batchSampleCount > 0 || hasMax || hasMin || hasMean || hasStd) {
      bool wasUnfinished = !hasVc && !(hasMax || hasMin || hasMean || hasStd);
      printFullBatch(wasUnfinished);
    } else {
      resetBatchBuffer();
    }
    return;
  }

  double valMm = parseRawMm(valRaw);

  if (code == "FF01") {
    statMaxMm = valMm;
    hasMax = true;
    lastStatPacketTime = millis();
  } else if (code == "FF02") {
    statMinMm = valMm;
    hasMin = true;
    lastStatPacketTime = millis();
  } else if (code == "FF03") {
    statMeanMm = valMm;
    hasMean = true;
    lastStatPacketTime = millis();
  } else if (code == "FF04") {
    statStdMm = valMm;
    hasStd = true;
    lastStatPacketTime = millis();
  } else if (code == "FF05") {
    statVcPct = valMm;
    hasVc = true;
    printFullBatch(false);
  } else {
    int sampleNum = code.toInt();
    if (sampleNum <= 0) return;

    int lastSampleNum = (batchSampleCount > 0) ? batchSamples[batchSampleCount - 1].num : 0;
    if (hasMax || hasMin || hasMean || hasStd) {
      printFullBatch(false);
    } else if (batchSampleCount > 0 && sampleNum <= lastSampleNum) {
      printFullBatch(true);
    }

    if (batchSampleCount < MAX_SAMPLES) {
      batchSamples[batchSampleCount].num   = sampleNum;
      batchSamples[batchSampleCount].valMm = valMm;
      batchSampleCount++;
    }
  }
}

void runTestPrint() {
  PrintConfig c = getConfigSnapshot();
  bool hasCirc = (c.circ > 0.0);

  printHeader(c);
  double s1Mm = 5.79, s2Mm = 5.74, s3Mm = 5.71, s4Mm = 5.69;
  appendSample2Col(1, s1Mm, hasCirc, c.circ, c.dec);
  appendSample2Col(2, s2Mm, hasCirc, c.circ, c.dec);
  appendSample2Col(3, s3Mm, hasCirc, c.circ, c.dec);
  appendSample2Col(4, s4Mm, hasCirc, c.circ, c.dec);
  flushPendingSample();
  printerPrintln("------------------------------------------");

  printerPrintln("MAX : " + formatDualValue(s1Mm, hasCirc, c.circ, c.dec));
  printerPrintln("MIN : " + formatDualValue(s4Mm, hasCirc, c.circ, c.dec));
  printerPrintln("MEAN: " + formatDualValue(5.7325, hasCirc, c.circ, c.dec));
  printerPrintln("STD : " + formatDualValue(0.0435, hasCirc, c.circ, 4));
  printerPrintln("COEFFICIENT OF VARIATION (V.C.): 0.75%");
  cutPaper();
}

// ===== TAC VU MANG CHAY NGAM TREN CORE 0 (KHONG CHAN CORE 1) =====
void syncFirebaseTask(void* pvParameters) {
  unsigned long lastReconnectAttempt = 0;

  for (;;) {
    if (isScanningWifi) {
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    if (WiFi.status() != WL_CONNECTED) {
      if (wifiSsid.length() > 0 && wifiSsid != DEFAULT_WIFI_SSID && (millis() - lastReconnectAttempt > 10000)) {
        lastReconnectAttempt = millis();
        WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!ntpConfigured) {
      configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
      ntpConfigured = true;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(3500);

    bool needClear = false;
    uint32_t clearEpochSnap = 0;
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    needClear = pendingClearAfterPrint;
    clearEpochSnap = clearConfigEpoch;
    xSemaphoreGive(cfgMutex);

    if (needClear) {
      int patchCode = -1;
      if (http.begin(client, FIREBASE_URL)) {
        http.addHeader("Content-Type", "application/json");
        patchCode = http.PATCH("{\"sample\":null,\"name\":null,\"type\":null,\"subType\":null,\"circ\":null,\"isManualCirc\":null}");
        http.end();
      }
      if (patchCode >= 200 && patchCode < 300) {
        xSemaphoreTake(cfgMutex, portMAX_DELAY);
        if (clearConfigEpoch == clearEpochSnap) {
          pendingClearAfterPrint = false;
        }
        xSemaphoreGive(cfgMutex);
        Serial.println("[DD60A] Da dong bo reset len Firebase.");
      } else {
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    uint32_t epochBeforeGet = 0;
    xSemaphoreTake(cfgMutex, portMAX_DELAY);
    epochBeforeGet = clearConfigEpoch;
    xSemaphoreGive(cfgMutex);

    if (http.begin(client, FIREBASE_URL)) {
      int httpCode = http.GET();
      if (httpCode == 200) {
        String payload = http.getString();
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
          PrintConfig nextCfg = getConfigSnapshot();
          bool changed = false;

          String vSample = doc["sample"].isNull() ? "" : doc["sample"].as<String>();
          if (vSample != nextCfg.sample) { nextCfg.sample = vSample; changed = true; }

          String vName = doc["name"].isNull() ? "" : doc["name"].as<String>();
          if (vName != nextCfg.name) { nextCfg.name = vName; changed = true; }

          String vType = doc["type"].isNull() ? "" : doc["type"].as<String>();
          if (vType != nextCfg.type) { nextCfg.type = vType; changed = true; }

          String vSubType = doc["subType"].isNull() ? "" : doc["subType"].as<String>();
          if (vSubType != nextCfg.subType) { nextCfg.subType = vSubType; changed = true; }

          double vCirc = doc["circ"].isNull() ? 0.0 : doc["circ"].as<double>();
          if (vCirc < 0.0) vCirc = 0.0;
          if (fabs(vCirc - nextCfg.circ) > 0.0001) { nextCfg.circ = vCirc; changed = true; }

          bool vManual = doc["isManualCirc"].isNull() ? false : doc["isManualCirc"].as<bool>();
          if (vManual != nextCfg.isManualCirc) { nextCfg.isManualCirc = vManual; changed = true; }

          int vDec = doc["dec"].isNull() ? 2 : doc["dec"].as<int>();
          if (vDec != nextCfg.dec) { nextCfg.dec = vDec; changed = true; }

          bool applied = false;
          bool allowApply = false;
          xSemaphoreTake(cfgMutex, portMAX_DELAY);
          if (!pendingClearAfterPrint && clearConfigEpoch == epochBeforeGet) {
            allowApply = true;
            if (changed) {
              cfg = nextCfg;
              applied = true;
            }
          }
          xSemaphoreGive(cfgMutex);

          if (applied) {
            saveConfigToNVS(nextCfg);
          }

          if (allowApply && doc["testPrint"] == true) {
            pendingTestPrint = true;
            http.end();
            if (http.begin(client, FIREBASE_URL)) {
              http.addHeader("Content-Type", "application/json");
              http.PATCH("{\"testPrint\":false}");
            }
          }
        }
      }
      http.end();
    }

    for (int w = 0; w < 15; w++) {
      if (pendingClearAfterPrint) break;
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  }
}

uint8_t scanI2cPair(int sda, int scl) {
  Wire.end();
  Wire.begin(sda, scl);
  Wire.setClock(100000);
  delay(20);
  uint8_t found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C FOUND (SDA=%d, SCL=%d): 0x%02X\n", sda, scl, addr);
      if (addr == 0x3C || addr == 0x3D) found = addr;
    }
  }
  return found;
}

void initOled() {
  delay(150); // Cho OLED on dinh nguon sau khi cap dien
  uint8_t foundAddr = scanI2cPair(OLED_SDA_PIN, OLED_SCL_PIN);
  if (foundAddr == 0) {
    // Thu dao cap chan 21 <-> 22 phong truong hop cam nguoc SDA/SCK
    foundAddr = scanI2cPair(OLED_SCL_PIN, OLED_SDA_PIN);
  }
  if (foundAddr == 0) {
    Wire.end();
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    Wire.setClock(100000);
    Serial.println("I2C OLED NOT FOUND on (21,22) or (22,21)");
    foundAddr = OLED_ADDR;
  }

  if (display.begin(SSD1306_SWITCHCAPVCC, foundAddr)) {
    oledReady = true;
    Serial.printf("OLED READY at 0x%02X\n", foundAddr);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setCursor(0, 12);
    display.println("DD60A BRIDGE READY");
    display.display();
  } else {
    oledReady = false;
    Serial.println("OLED BEGIN FAILED");
  }
}

void updateOledDisplay(bool force) {
  if (!oledReady) return;

  PrintConfig c = getConfigSnapshot();
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  String key = c.type + "|" + c.subType + "|" + String(c.circ, 2) + "|" +
               String(c.isManualCirc) + "|" + String(wifiOk) + "|" + String(batchSampleCount);
  if (!force && key == lastOledKey) return;
  lastOledKey = key;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  // Hang 1 (y=0, co 1): Loai & Phan loai + trang thai Wi-Fi / So mau
  display.setTextSize(1);
  display.setCursor(0, 0);
  if (c.circ > 0.0) {
    String label = c.type + ":" + (c.isManualCirc ? "Nhap tay" : c.subType);
    if (label.length() > 17) label = label.substring(0, 17);
    display.print(label);
  } else {
    display.print("Loai: null");
  }

  display.setCursor(104, 0);
  if (batchSampleCount > 0) {
    display.print("D:");
    display.print(batchSampleCount);
  } else {
    display.print(wifiOk ? "[OK]" : "[OF]");
  }
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  // Hang 2 (y=14, co 2 - chu to): Kich co chu vi
  display.setTextSize(2);
  display.setCursor(0, 14);
  if (c.circ > 0.0) {
    display.print(formatRounded(c.circ, 2));
    display.print(" mm");
  } else {
    display.print("null");
  }

  display.display();
}

void updateWifiLeds() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  digitalWrite(LED_GREEN_PIN, wifiOk ? HIGH : LOW);
  digitalWrite(LED_RED_PIN,   wifiOk ? LOW  : HIGH);
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(2400, SERIAL_7N1, DD60A_RX_PIN, DD60A_TX_PIN);

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  updateWifiLeds();

  cfgMutex = xSemaphoreCreateMutex();
  loadConfigFromNVS();
  loadWifiFromNVS();
  loadPrinterConfigFromNVS();
  applyPrinterSerial();

  initOled();
  updateOledDisplay(true);

  // Bat AP+STA song song: luon co the ket noi "DD60A-Setup" de doi Wi-Fi & cau hinh may in
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);
  if (wifiSsid.length() > 0 && wifiSsid != DEFAULT_WIFI_SSID) {
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }

  // Captive Portal DNS + WebServer
  dnsServer.start(53, "*", WiFi.softAPIP());
  webServer.on("/", HTTP_GET, handlePortalRoot);
  webServer.on("/save", HTTP_POST, handlePortalSave);
  webServer.on("/save-printer", HTTP_POST, handlePortalSavePrinter);
  webServer.onNotFound(handlePortalNotFound);
  webServer.begin();

  Serial.println("He thong san sang! AP: DD60A-Setup (IP: 192.168.4.1)");
  Serial.println("DD60A Serial1 READY: 2400 7-N-1 (RX=GPIO16, TX=GPIO17) [RAW+PKT DEBUG ON]");

  // Chay tac vu Wi-Fi / Firebase o Core 0 de Core 1 chuyen tam nhan DD60A & in
  xTaskCreatePinnedToCore(syncFirebaseTask, "FirebaseTask", 8192, NULL, 1, NULL, 0);
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  checkPrinterFlowControl();

  while (Serial1.available()) {
    byte raw = Serial1.read();
    byte b = raw & 0x7F;
    if (!rawDebugLineOpen) {
      Serial.print("[DD60A RAW]");
      rawDebugLineOpen = true;
    }
    Serial.printf(" %02X", raw);
    if (b >= 32 && b <= 126) Serial.printf("(%c)", (char)b);
    lastRawRxTime = millis();

    if (b == 0x02) {
      packetBuffer = "";
      receivingPacket = true;
    } else if (b == 0x03) {
      if (receivingPacket) {
        processPacket(packetBuffer);
        receivingPacket = false;
      }
    } else if (receivingPacket) {
      packetBuffer += (char)b;
      if (packetBuffer.length() > 30) receivingPacket = false;
    }
  }

  if (rawDebugLineOpen && (millis() - lastRawRxTime > 150)) {
    Serial.println();
    rawDebugLineOpen = false;
  }

  // Fallback: neu do 1 mau ma may dung o FF01..FF04 khong gui FF05, tu in sau 6000ms
  // (Thuc te DD60A vua in kim vua gui FF01->FF05 cach nhau 2.1s - 3.3s moi goi)
  if (!receivingPacket && !Serial1.available() && lastStatPacketTime > 0 && (millis() - lastStatPacketTime > 6000)) {
    printFullBatch(false);
  }

  if (pendingTestPrint && !receivingPacket && !Serial1.available() && batchSampleCount == 0 && lastStatPacketTime == 0) {
    pendingTestPrint = false;
    runTestPrint();
  }

  if (!receivingPacket && !Serial1.available() && (millis() - lastOledCheck > 500)) {
    lastOledCheck = millis();
    updateWifiLeds();
    updateOledDisplay(false);
  }
}
