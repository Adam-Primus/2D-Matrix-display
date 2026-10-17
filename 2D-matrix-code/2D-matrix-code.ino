#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// -------WiFi
const char *WIFI_SSID = "WIFI_NAME";
const char *WIFI_PASS = "WIFI_PASSWORD";
const char *AP_SSID   = "LED-Matrix";
const char *AP_PASS   = "12345678";

const uint8_t ROWS = 5;
const uint8_t COLS = 6;
const uint8_t rowPins[ROWS] = {18, 19, 21, 22, 23};
const uint8_t colPins[COLS] = {14, 27, 26, 25, 33, 32};

#define ROW_ON   HIGH
#define ROW_OFF  LOW
#define COL_ON   HIGH
#define COL_OFF  LOW
#define ROW_TICKS 1

volatile uint8_t frame[ROWS];

const uint8_t FONT[41][5] = {
  {0b01110, 0b10001, 0b11111, 0b10001, 0b10001}, // A
  {0b11110, 0b10001, 0b11110, 0b10001, 0b11110}, // B
  {0b01111, 0b10000, 0b10000, 0b10000, 0b01111}, // C
  {0b11110, 0b10001, 0b10001, 0b10001, 0b11110}, // D
  {0b11111, 0b10000, 0b11110, 0b10000, 0b11111}, // E
  {0b11111, 0b10000, 0b11110, 0b10000, 0b10000}, // F
  {0b01111, 0b10000, 0b10011, 0b10001, 0b01111}, // G
  {0b10001, 0b10001, 0b11111, 0b10001, 0b10001}, // H
  {0b11111, 0b00100, 0b00100, 0b00100, 0b11111}, // I
  {0b00111, 0b00010, 0b00010, 0b10010, 0b01100}, // J
  {0b10001, 0b10010, 0b11100, 0b10010, 0b10001}, // K
  {0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, // L
  {0b10001, 0b11011, 0b10101, 0b10001, 0b10001}, // M
  {0b10001, 0b11001, 0b10101, 0b10011, 0b10001}, // N
  {0b01110, 0b10001, 0b10001, 0b10001, 0b01110}, // O
  {0b11110, 0b10001, 0b11110, 0b10000, 0b10000}, // P
  {0b01110, 0b10001, 0b10101, 0b10010, 0b01101}, // Q
  {0b11110, 0b10001, 0b11110, 0b10010, 0b10001}, // R
  {0b01111, 0b10000, 0b01110, 0b00001, 0b11110}, // S
  {0b11111, 0b00100, 0b00100, 0b00100, 0b00100}, // T
  {0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // U
  {0b10001, 0b10001, 0b10001, 0b01010, 0b00100}, // V
  {0b10001, 0b10001, 0b10101, 0b11011, 0b10001}, // W
  {0b10001, 0b01010, 0b00100, 0b01010, 0b10001}, // X
  {0b10001, 0b01010, 0b00100, 0b00100, 0b00100}, // Y
  {0b11111, 0b00010, 0b00100, 0b01000, 0b11111}, // Z
  {0b01110, 0b10011, 0b10101, 0b11001, 0b01110}, // 0
  {0b00100, 0b01100, 0b00100, 0b00100, 0b01110}, // 1
  {0b11110, 0b00001, 0b01110, 0b10000, 0b11111}, // 2
  {0b11110, 0b00001, 0b01110, 0b00001, 0b11110}, // 3
  {0b10010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
  {0b11111, 0b10000, 0b11110, 0b00001, 0b11110}, // 5
  {0b01110, 0b10000, 0b11110, 0b10001, 0b01110}, // 6
  {0b11111, 0b00001, 0b00010, 0b00100, 0b00100}, // 7
  {0b01110, 0b10001, 0b01110, 0b10001, 0b01110}, // 8
  {0b01110, 0b10001, 0b01111, 0b00001, 0b01110}, // 9
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00000}, // space
  {0b00000, 0b00000, 0b01110, 0b00000, 0b00000}, // -
  {0b00000, 0b00000, 0b00000, 0b00000, 0b00100}, // .
  {0b00100, 0b00100, 0b00100, 0b00000, 0b00100}, // !
  {0b01110, 0b00001, 0b00110, 0b00000, 0b00100}  // ?
};

int glyphIndex(char ch) {
  ch = toupper(ch);
  if (ch >= 'A' && ch <= 'Z') return ch - 'A';
  if (ch >= '0' && ch <= '9') return 26 + (ch - '0');
  switch (ch) {
    case '-': return 37;
    case '.': return 38;
    case '!': return 39;
    case '?': return 40;
  }
  return 36;
}

void clearFrame() { for (uint8_t r = 0; r < ROWS; r++) frame[r] = 0; }
void fillFrame()  { for (uint8_t r = 0; r < ROWS; r++) frame[r] = 0x3F; }

void setPixel(uint8_t row, uint8_t col, bool on = true) {
  if (row >= ROWS || col >= COLS) return;
  if (on) frame[row] |=  (1 << col);
  else    frame[row] &= ~(1 << col);
}

void setLedByNumber(uint8_t n, bool on = true) {
  if (n < 1 || n > ROWS * COLS) return;
  uint8_t idx = n - 1;
  setPixel(idx % ROWS, idx / ROWS, on);
}

void showChar(char ch) {
  clearFrame();
  const uint8_t *g = FONT[glyphIndex(ch)];
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < 5; c++)
      if (g[r] & (1 << (4 - c))) setPixel(r, c, true);
}

void drawBitmap(const char *lines[ROWS]) {
  clearFrame();
  for (uint8_t r = 0; r < ROWS; r++)
    for (uint8_t c = 0; c < COLS; c++)
      if (lines[r][c] == 'X') setPixel(r, c, true);
}

void refreshTask(void *param) {
  for (;;) {
    for (uint8_t r = 0; r < ROWS; r++) {
      for (uint8_t i = 0; i < ROWS; i++) digitalWrite(rowPins[i], ROW_OFF);
      uint8_t bits = frame[r];
      for (uint8_t c = 0; c < COLS; c++)
        digitalWrite(colPins[c], (bits & (1 << c)) ? COL_ON : COL_OFF);
      digitalWrite(rowPins[r], ROW_ON);
      vTaskDelay(ROW_TICKS);
    }
  }
}

enum Mode { MODE_MANUAL, MODE_DEMO, MODE_TEXT };
Mode mode = MODE_MANUAL;

String   textStr;
bool     textStep   = false;
uint16_t textSpeed  = 150;
int      textPos    = 0;
bool     stepBlank  = false;
uint32_t textLast   = 0;
uint32_t textDelay  = 0;

int demoPattern = -1;

WebServer server(80);

uint8_t textColumn(int idx) {
  int len = textStr.length();
  if (idx < 0 || idx >= len * 6) return 0;
  int ch  = idx / 6;
  int sub = idx % 6;
  if (sub == 5) return 0;                     // gap between letters
  const uint8_t *g = FONT[glyphIndex(textStr[ch])];
  uint8_t col = 0;
  for (uint8_t r = 0; r < ROWS; r++)
    if (g[r] & (1 << (4 - sub))) col |= (1 << r);
  return col;
}

void runTextStep() {
  uint32_t now = millis();
  if (now - textLast < textDelay) return;
  textLast = now;
  int len = textStr.length();
  if (len == 0) { clearFrame(); return; }

  if (textStep) {
    if (stepBlank) {
      clearFrame();
      stepBlank = false;
      textDelay = 100;
      return;
    }
    if (textPos >= len) textPos = 0;
    showChar(textStr[textPos++]);
    stepBlank = true;
    textDelay = textSpeed;
  } else {
    int total = len * 6;
    for (uint8_t r = 0; r < ROWS; r++) {
      uint8_t bits = 0;
      for (uint8_t c = 0; c < COLS; c++)
        if (textColumn(textPos - COLS + c) & (1 << r)) bits |= (1 << c);
      frame[r] = bits;
    }
    textPos++;
    if (textPos > total + COLS) textPos = 0;
    textDelay = textSpeed;
  }
}

bool pauseMs(uint32_t ms) {
  uint32_t t = millis();
  while (millis() - t < ms) {
    server.handleClient();
    if (mode != MODE_DEMO) return false;
    delay(1);
  }
  return true;
}

void patBlink() {
  for (int i = 0; i < 3; i++) {
    fillFrame();  if (!pauseMs(300)) return;
    clearFrame(); if (!pauseMs(300)) return;
  }
}
void patChase() {
  for (uint8_t n = 1; n <= 30; n++) {
    clearFrame(); setLedByNumber(n);
    if (!pauseMs(80)) return;
  }
  clearFrame();
}
void patRowScan() {
  for (uint8_t r = 0; r < ROWS; r++) { clearFrame(); frame[r] = 0x3F; if (!pauseMs(200)) return; }
  for (int r = ROWS - 2; r >= 0; r--) { clearFrame(); frame[r] = 0x3F; if (!pauseMs(200)) return; }
  clearFrame();
}
void patColScan() {
  for (uint8_t c = 0; c < COLS; c++) {
    clearFrame(); for (uint8_t r = 0; r < ROWS; r++) setPixel(r, c);
    if (!pauseMs(200)) return;
  }
  for (int c = COLS - 2; c >= 0; c--) {
    clearFrame(); for (uint8_t r = 0; r < ROWS; r++) setPixel(r, c);
    if (!pauseMs(200)) return;
  }
  clearFrame();
}
void patChecker() {
  for (int i = 0; i < 6; i++) {
    clearFrame();
    for (uint8_t r = 0; r < ROWS; r++)
      for (uint8_t c = 0; c < COLS; c++)
        if (((r + c + i) & 1) == 0) setPixel(r, c);
    if (!pauseMs(300)) return;
  }
  clearFrame();
}
void patBox() {
  for (uint8_t k = 0; k < 3; k++) {
    clearFrame();
    int r0 = 2 - k, r1 = 2 + k, c0 = 2 - k, c1 = 3 + k;
    for (int r = r0; r <= r1; r++)
      for (int c = c0; c <= c1; c++)
        if ((r == r0 || r == r1 || c == c0 || c == c1) && r >= 0 && r < ROWS && c >= 0 && c < COLS)
          setPixel(r, c);
    if (!pauseMs(250)) return;
  }
  clearFrame();
}
void patSparkle() {
  for (int i = 0; i < 40; i++) {
    setPixel(random(ROWS), random(COLS), random(2));
    if (!pauseMs(80)) return;
  }
  clearFrame();
}
void patHeart() {
  const char *heart[ROWS] = {".XX.XX", "XXXXXX", "XXXXXX", ".XXXX.", "..XX.."};
  for (int i = 0; i < 3; i++) {
    drawBitmap(heart); if (!pauseMs(400)) return;
    clearFrame();      if (!pauseMs(250)) return;
  }
}

const uint8_t PATTERN_COUNT = 8;
void runPattern(int n) {
  switch (n) {
    case 0: patBlink();   break;
    case 1: patChase();   break;
    case 2: patRowScan(); break;
    case 3: patColScan(); break;
    case 4: patChecker(); break;
    case 5: patBox();     break;
    case 6: patSparkle(); break;
    case 7: patHeart();   break;
  }
}

// --------Website
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 LED Matrix</title>
<style>
  :root{--bg:#10131a;--card:#1a1f2b;--line:#2a3142;--txt:#e8ecf4;--mut:#8b95ab;--acc:#ff4d4d;--btn:#252c3d}
  *{box-sizing:border-box}
  body{margin:0;background:var(--bg);color:var(--txt);font-family:system-ui,Segoe UI,Roboto,sans-serif;padding:16px}
  .wrap{max-width:620px;margin:0 auto;display:grid;gap:14px}
  h1{font-size:20px;margin:0}
  .sub{color:var(--mut);font-size:13px;margin-top:2px}
  .card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px}
  .card h2{font-size:13px;letter-spacing:.06em;text-transform:uppercase;color:var(--mut);margin:0 0 10px}
  #grid{display:grid;grid-template-columns:auto repeat(6,1fr);gap:8px;align-items:center;justify-items:center}
  .hd{font-size:11px;color:var(--mut);text-align:center;line-height:1.25}
  .rl{font-size:11px;color:var(--mut);text-align:right;line-height:1.25}
  .led{width:100%;max-width:56px;aspect-ratio:1;border-radius:50%;border:2px solid #3a4258;background:#222838;cursor:pointer;padding:0;transition:.12s}
  .led.on{background:var(--acc);border-color:#ff9a9a;box-shadow:0 0 14px 2px rgba(255,77,77,.7)}
  .row{display:flex;flex-wrap:wrap;gap:8px}
  button.b{background:var(--btn);color:var(--txt);border:1px solid var(--line);border-radius:10px;padding:9px 14px;font-size:14px;cursor:pointer}
  button.b:hover{border-color:#566083}
  button.b:active{transform:scale(.97)}
  button.acc{background:var(--acc);border-color:var(--acc);color:#fff;font-weight:600}
  .keys button{min-width:42px;padding:9px 0;font-weight:600}
  input[type=text],select{background:#121620;color:var(--txt);border:1px solid var(--line);border-radius:10px;padding:10px;font-size:15px}
  input[type=text]{flex:1;min-width:140px;text-transform:uppercase}
  input[type=range]{flex:1;min-width:120px;accent-color:var(--acc)}
  .st{font-size:12px;color:var(--mut)}
  .dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:#555;margin-right:6px}
  .dot.ok{background:#3ddc84}
</style>
</head>
<body>
<div class="wrap">
  <div>
    <h1>ESP32 LED Matrix</h1>
    <div class="sub"><span id="dot" class="dot"></span><span id="status">connecting...</span></div>
  </div>

  <div class="card">
    <h2>LEDs (tap to toggle)</h2>
    <div id="grid"></div>
    <div class="row" style="margin-top:14px">
      <button class="b" onclick="api('/clear')">Clear</button>
      <button class="b" onclick="api('/fill')">All on</button>
      <button class="b" onclick="api('/invert')">Invert</button>
    </div>
  </div>

  <div class="card">
    <h2>Letters, numbers &amp; symbols</h2>
    <div class="row keys" id="keys"></div>
  </div>

  <div class="card">
    <h2>Text</h2>
    <div class="row">
      <input type="text" id="txt" maxlength="60" placeholder="Type text, e.g. HELLO 2026" value="HELLO">
      <select id="style">
        <option value="scroll">Scroll</option>
        <option value="step">One by one</option>
      </select>
    </div>
    <div class="row" style="margin-top:10px;align-items:center">
      <span class="st">Fast</span>
      <input type="range" id="spd" min="40" max="1500" value="150">
      <span class="st">Slow</span>
      <span class="st" id="spdv" style="min-width:56px;text-align:right">150 ms</span>
    </div>
    <div class="row" style="margin-top:10px">
      <button class="b acc" onclick="sendText()">Show text</button>
      <button class="b" onclick="api('/stop')">Stop</button>
    </div>
  </div>

  <div class="card">
    <h2>Animations</h2>
    <div class="row" id="pats"></div>
  </div>
</div>

<script>
const R=5,C=6;
const rowPins=[18,19,21,22,23], colPins=[14,27,26,25,33,32];
let f=[0,0,0,0,0], cells=[];

// grid
const g=document.getElementById('grid');
g.appendChild(document.createElement('div'));
colPins.forEach((p,c)=>{const d=document.createElement('div');d.className='hd';d.innerHTML='C'+(c+1)+'<br>GPIO'+p;g.appendChild(d);});
for(let r=0;r<R;r++){
  const l=document.createElement('div');l.className='rl';l.innerHTML='R'+(r+1)+'<br>GPIO'+rowPins[r];g.appendChild(l);
  cells[r]=[];
  for(let c=0;c<C;c++){
    const b=document.createElement('button');b.className='led';
    b.onclick=()=>{f[r]^=(1<<c);draw();api('/frame?d='+f.join(','));};
    g.appendChild(b);cells[r][c]=b;
  }
}
function draw(){for(let r=0;r<R;r++)for(let c=0;c<C;c++)cells[r][c].classList.toggle('on',!!(f[r]&(1<<c)));}

// keys
const keys=document.getElementById('keys');
"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!?".split('').forEach(ch=>{
  const b=document.createElement('button');b.className='b';b.textContent=ch;
  b.onclick=()=>api('/char?c='+encodeURIComponent(ch));keys.appendChild(b);
});

// patterns
const pats=document.getElementById('pats');
[['Demo (all)',-1],['Blink',0],['Chase',1],['Row scan',2],['Column scan',3],['Checkerboard',4],['Box',5],['Sparkle',6],['Heart',7]]
.forEach(([n,i])=>{const b=document.createElement('button');b.className='b'+(i<0?' acc':'');b.textContent=n;b.onclick=()=>api('/demo?p='+i);pats.appendChild(b);});

// text
const spd=document.getElementById('spd'),spdv=document.getElementById('spdv'),sty=document.getElementById('style');
spd.oninput=()=>spdv.textContent=spd.value+' ms';
sty.onchange=()=>{spd.value=sty.value==='step'?600:150;spd.oninput();};
function sendText(){
  const t=document.getElementById('txt').value.trim();
  if(!t)return;
  api('/text?t='+encodeURIComponent(t)+'&speed='+spd.value+'&style='+sty.value);
}
document.getElementById('txt').addEventListener('keydown',e=>{if(e.key==='Enter')sendText();});

// comms
async function api(u){
  try{await fetch(u);}catch(e){setStatus(false,'connection lost');return;}
  refresh();
}
function setStatus(ok,msg){document.getElementById('dot').className='dot'+(ok?' ok':'');document.getElementById('status').textContent=msg;}
async function refresh(){
  try{
    const j=await (await fetch('/state')).json();
    f=j.f;draw();
    setStatus(true,'connected - mode: '+j.m);
  }catch(e){setStatus(false,'connection lost');}
}
refresh();setInterval(refresh,400);
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send_P(200, "text/html", INDEX_HTML); }

void handleState() {
  const char *m = (mode == MODE_MANUAL) ? "manual" : (mode == MODE_DEMO) ? "animation" : "text";
  String s = "{\"m\":\"";
  s += m;
  s += "\",\"f\":[";
  for (uint8_t r = 0; r < ROWS; r++) {
    s += (int)frame[r];
    if (r < ROWS - 1) s += ",";
  }
  s += "]}";
  server.send(200, "application/json", s);
}

void handleFrame() {
  if (!server.hasArg("d")) { server.send(400, "text/plain", "missing d"); return; }
  String d = server.arg("d");
  uint8_t tmp[ROWS] = {0, 0, 0, 0, 0};
  int start = 0;
  for (uint8_t r = 0; r < ROWS; r++) {
    int comma = d.indexOf(',', start);
    String part = (comma < 0) ? d.substring(start) : d.substring(start, comma);
    tmp[r] = (uint8_t)(part.toInt() & 0x3F);
    if (comma < 0) break;
    start = comma + 1;
  }
  mode = MODE_MANUAL;
  for (uint8_t r = 0; r < ROWS; r++) frame[r] = tmp[r];
  server.send(200, "text/plain", "ok");
}

void handleClear()  { mode = MODE_MANUAL; clearFrame(); server.send(200, "text/plain", "ok"); }
void handleFill()   { mode = MODE_MANUAL; fillFrame();  server.send(200, "text/plain", "ok"); }
void handleInvert() {
  mode = MODE_MANUAL;
  for (uint8_t r = 0; r < ROWS; r++) frame[r] = (~frame[r]) & 0x3F;
  server.send(200, "text/plain", "ok");
}
void handleStop()   { mode = MODE_MANUAL; clearFrame(); server.send(200, "text/plain", "ok"); }

void handleChar() {
  String c = server.arg("c");
  if (c.length() == 0) { server.send(400, "text/plain", "missing c"); return; }
  mode = MODE_MANUAL;
  showChar(c[0]);
  server.send(200, "text/plain", "ok");
}

void handleText() {
  String t = server.arg("t");
  t.toUpperCase();
  if (t.length() > 60) t = t.substring(0, 60);
  textStr   = t;
  textStep  = (server.arg("style") == "step");
  textSpeed = constrain(server.arg("speed").toInt(), 40, 2000);
  if (textSpeed == 0) textSpeed = 150;
  textPos   = 0;
  stepBlank = false;
  textLast  = 0;
  textDelay = 0;
  mode = MODE_TEXT;
  server.send(200, "text/plain", "ok");
}

void handleDemo() {
  int p = server.hasArg("p") ? server.arg("p").toInt() : -1;
  demoPattern = (p >= 0 && p < PATTERN_COUNT) ? p : -1;
  clearFrame();
  mode = MODE_DEMO;
  server.send(200, "text/plain", "ok");
}

// ---------------- Setup / loop
void startWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Open: http://");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("WiFi failed - hotspot \"");
    Serial.print(AP_SSID);
    Serial.print("\" started. Open: http://");
    Serial.println(WiFi.softAPIP());
  }
  if (MDNS.begin("ledmatrix")) Serial.println("Also: http://ledmatrix.local");
}

void setup() {
  Serial.begin(115200);

  for (uint8_t r = 0; r < ROWS; r++) { pinMode(rowPins[r], OUTPUT); digitalWrite(rowPins[r], ROW_OFF); }
  for (uint8_t c = 0; c < COLS; c++) { pinMode(colPins[c], OUTPUT); digitalWrite(colPins[c], COL_OFF); }
  clearFrame();
  randomSeed(micros());

  xTaskCreatePinnedToCore(refreshTask, "matrixRefresh", 2048, NULL, 2, NULL, 0);

  startWiFi();

  server.on("/",       handleRoot);
  server.on("/state",  handleState);
  server.on("/frame",  handleFrame);
  server.on("/clear",  handleClear);
  server.on("/fill",   handleFill);
  server.on("/invert", handleInvert);
  server.on("/stop",   handleStop);
  server.on("/char",   handleChar);
  server.on("/text",   handleText);
  server.on("/demo",   handleDemo);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();

  showChar('O'); delay(500);
  showChar('K'); delay(500);
  clearFrame();
}

void loop() {
  server.handleClient();

  if (mode == MODE_TEXT) {
    runTextStep();
  } else if (mode == MODE_DEMO) {
    if (demoPattern < 0) {
      for (uint8_t i = 0; i < PATTERN_COUNT && mode == MODE_DEMO; i++) runPattern(i);
    } else {
      runPattern(demoPattern);
    }
  }
  delay(1);
}
