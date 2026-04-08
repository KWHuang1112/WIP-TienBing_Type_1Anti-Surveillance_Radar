#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>// screen switch

#define TFT_CS 7
#define TFT_DC 2
#define TFT_RST 5
#define TFT_MOSI 4
#define TFT_SCLK 3
#define TRIG_PIN 0
#define ECHO_PIN 1
#define SERVO_PIN 10

const char* ssid = "Radar_System";
const char* password = "password123";

// 在 loop() 外宣告變數防止重複觸發
unsigned long lastTriggerTime = 0;// screen switch
bool isTriggered = false;// screen switch

WebServer server(80);
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
Servo myServo;

int currentAngle = 0;
int currentDistance = 0;
int lastX[181], lastY[181]; 
int lastDist[181];  // 新增：儲存上一次的距離數值
float filteredDist = 0; 

bool autoScan = true;
int manualAngle = 90;

long getDistance() {
  long samples[3];
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long duration = pulseIn(ECHO_PIN, HIGH, 24000); 
    samples[i] = (duration == 0) ? 400 : duration * 0.034 / 2;
  }
  if (samples[0] > samples[1]) std::swap(samples[0], samples[1]);
  if (samples[1] > samples[2]) std::swap(samples[1], samples[2]);
  if (samples[0] > samples[1]) std::swap(samples[0], samples[1]);
  long median = samples[1];
  float alpha = 0.6; 
  filteredDist = (alpha * median) + (1.0 - alpha) * filteredDist;
  return (long)filteredDist;
}

// String getHTML() {
//   return R"rawhtml(
// <!DOCTYPE html><html><head><meta charset='UTF-8'>
// <meta name='viewport' content='width=device-width,initial-scale=1'>
// <style>
//   body{background:#000;color:#0f0;font-family:monospace;text-align:center;margin:0;overflow:hidden}
//   canvas{background:#000;border-bottom:2px solid #030;width:95vw;max-width:500px}
//   #i{font-size:1.2rem;margin-top:10px;color:#0ff}
// </style></head>
// <body>
//   <h3>400CM RADAR MONITOR</h3>
//   <canvas id='c' width='500' height='280'></canvas>
//   <div id='i'>ANGLE: <span id='a'>0</span>° | DIST: <span id='d'>0</span>cm</div>

// <script>
//   const cvs=document.getElementById('c'), ctx=cvs.getContext('2d');
//   const cx=250, cy=270, rM=250;
//   let lastDistMap = new Array(181).fill(400); // 紀錄網頁端的舊距離

//   function drawBackground(){
//     ctx.fillStyle='rgba(0,10,0,0.1)'; // 餘暉效果
//     ctx.fillRect(0,0,500,300);
//     ctx.strokeStyle='#020';
//     ctx.lineWidth=1;
//     for(let r=62.5; r<=rM; r+=62.5){ // 繪製 100, 200, 300, 400cm 網格
//       ctx.beginPath(); ctx.arc(cx,cy,r,Math.PI,2*Math.PI); ctx.stroke();
//     }
//   }

//   setInterval(async()=>{
//     try{
//       let res=await fetch('/data'), j=await res.json();
//       document.getElementById('a').innerText=j.angle;
//       document.getElementById('d').innerText=j.distance;
      
//       drawBackground();
      
//       // 計算座標 (修正為 180-angle 保持方向一致)
//       let rad=(180-j.angle)*Math.PI/180;
//       let lx=cx-Math.cos(rad)*rM, ly=cy-Math.sin(rad)*rM;
      
//       // 繪製掃描線
//       ctx.strokeStyle='#0f0';
//       ctx.lineWidth=2;
//       ctx.beginPath(); ctx.moveTo(cx,cy); ctx.lineTo(lx,ly); ctx.stroke();
      
//       // 繪製偵測點
//       if(j.distance < 395){
//         let tx=cx-Math.cos(rad)*(j.distance*0.625);
//         let ty=cy-Math.sin(rad)*(j.distance*0.625);
        
//         // 判定顏色：與網頁端上一次紀錄比對
//         let diff = Math.abs(j.distance - lastDistMap[j.angle]);
//         ctx.fillStyle = (diff < 10) ? '#ff0' : '#f00'; // 穩定點黃色，新點紅色
        
//         ctx.beginPath(); ctx.arc(tx,ty,5,0,7); ctx.fill();
//         lastDistMap[j.angle] = j.distance; // 更新紀錄
//       }
//     }catch(e){}
//   }, 50); // 每 50ms 更新一次，配合 ESP32 節奏
// </script></body></html>)rawhtml";
// }

String getHTML() {
  return R"rawhtml(<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#000;color:#0f0;font-family:monospace;text-align:center;user-select:none;-webkit-user-select:none;}canvas{background:#000;border:1px solid #030;width:95vw;max-width:500px;border-radius:50% 50% 0 0;touch-action:none;}button{background:#030;color:#0f0;border:1px solid #0f0;padding:5px 15px;margin-top:10px;cursor:pointer;font-family:monospace;}input[type=range]{width:90%;max-width:400px;margin:15px 0;accent-color:#0f0;}</style></head><body><h3>400CM RADAR MONITOR</h3><canvas id='c' width='500' height='280'></canvas><div id='i'>ANG: <span id='a'>0</span> | DST: <span id='d'>0</span>cm</div><div><button id='btnAuto' onclick='setAuto(!autoMode)'>MODE: AUTO</button></div><div><input type='range' id='s' min='0' max='180' value='90'></div><script>const cvs=document.getElementById('c'),ctx=cvs.getContext('2d'),cx=250,cy=270,rM=250,slider=document.getElementById('s');
let manualAngle=90,autoMode=true,lastSend=0;
function setAuto(auto){autoMode=auto;document.getElementById('btnAuto').innerText=auto?'MODE: AUTO':'MODE: MANUAL';fetch('/setMode?auto='+(auto?'true':'false'));}
slider.addEventListener('input',e=>{if(autoMode)setAuto(false);manualAngle=parseInt(slider.value,10);document.getElementById('a').innerText=manualAngle;let now=Date.now();if(now-lastSend>80){lastSend=now;fetch('/setAngle?val='+manualAngle);}});
slider.addEventListener('change',e=>{fetch('/setAngle?val='+manualAngle);});
function draw(){ctx.fillStyle='rgba(0,15,0,0.1)';ctx.fillRect(0,0,500,300);ctx.strokeStyle='#020';for(let r=62.5;r<=rM;r+=62.5){ctx.beginPath();ctx.arc(cx,cy,r,Math.PI,2*Math.PI);ctx.stroke()}}
setInterval(async()=>{try{let res=await fetch('/data'),j=await res.json();if(autoMode){document.getElementById('a').innerText=j.angle;slider.value=j.angle;}document.getElementById('d').innerText=j.distance;draw();
let drawAng=autoMode?j.angle:manualAngle;let rad=(180-drawAng)*Math.PI/180;
let lx=cx-Math.cos(rad)*rM,ly=cy-Math.sin(rad)*rM;ctx.strokeStyle='#0f0';ctx.lineWidth=2;ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(lx,ly);ctx.stroke();
if(j.distance<395){let tx=cx-Math.cos(rad)*(j.distance*0.625),ty=cy-Math.sin(rad)*(j.distance*0.625);ctx.fillStyle='#f00';ctx.beginPath();ctx.arc(tx,ty,6,0,7);ctx.fill()}}catch(e){}},50);</script></body></html>)rawhtml";
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  
  // 初始化陣列
  for(int i=0; i<181; i++) { lastDist[i] = 400; lastX[i] = 0; }

  tft.drawCircle(80, 120, 15, 0x03E0); 
  tft.drawCircle(80, 120, 30, 0x03E0); 
  tft.drawCircle(80, 120, 45, 0x03E0);
  tft.drawCircle(80, 120, 60, 0x03E0); 

  WiFi.softAP(ssid, password);
  server.on("/", []() { server.send(200, "text/html", getHTML()); });
  server.on("/data", []() {
    String s = "{\"angle\":" + String(currentAngle) + ",\"distance\":" + String(currentDistance) + "}";
    server.send(200, "application/json", s);
  });
  server.on("/setMode", []() {
    if (server.hasArg("auto")) autoScan = (server.arg("auto") == "true");
    server.send(200, "text/plain", "OK");
  });
  server.on("/setAngle", []() {
    if (server.hasArg("val")) {
      manualAngle = server.arg("val").toInt();
      if (manualAngle < 0) manualAngle = 0;
      if (manualAngle > 180) manualAngle = 180;
    }
    server.send(200, "text/plain", "OK");
  });
  server.begin();
  myServo.attach(SERVO_PIN, 500, 2400);
}

void loop() {
  static int angle = 0;
  static int step = 6; // 進步馬達6度
  static int lastLineX = 80;
  static int lastLineY = 120;
  
  // 螢幕切換控制變數 (改為全域或 static)
  static bool pcIsTriggered = false;
  static unsigned long lastTriggerTime = 0;

  // 1. 硬體控制與測距
  myServo.write(angle);
  currentAngle = angle;
  currentDistance = getDistance(); 

  // 2. 座標設定 (180-angle)
  int x0 = 80, y0 = 120;
  float rad = (180 - angle) * PI / 180.0; 

  // --- 3. TFT 掃描繪圖 ---
  tft.drawLine(x0, y0, lastLineX, lastLineY, ST7735_BLACK); // 擦除舊線
  if (lastX[angle] != 0) {
    tft.fillCircle(lastX[angle], lastY[angle], 2, ST7735_BLACK); // 擦除舊點
  }

  int lx = x0 - cos(rad) * 60;
  int ly = y0 - sin(rad) * 60;
  tft.drawLine(x0, y0, lx, ly, ST7735_GREEN); // 畫新線
  lastLineX = lx;
  lastLineY = ly;

  if (currentDistance < 400) {
    float displayDist = (currentDistance / 400.0) * 60.0;
    int px = x0 - cos(rad) * displayDist;
    int py = y0 - sin(rad) * displayDist;
    uint16_t dotColor = (abs(currentDistance - lastDist[angle]) < 12) ? 0xFFE0 : ST7735_RED;
    tft.fillCircle(px, py, 2, dotColor);
    lastX[angle] = px;
    lastY[angle] = py;
    lastDist[angle] = currentDistance;
  } else {
    lastX[angle] = 0;
    lastDist[angle] = 400;
  }

  // --- 4. 網格與 UI 顯示 ---
  if (angle % 40 == 0) {
    tft.drawCircle(x0, y0, 30, 0x03E0); 
    tft.drawCircle(x0, y0, 60, 0x03E0); 
  }

  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
  tft.print("Link: 192.168.4.1");

  if (angle % 24 == 0) { //6 的倍數，每 4 步更新一次文字
    tft.setCursor(0, 10);
    tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
    tft.printf("ANG:%3d DST:%3dcm  ", currentAngle, currentDistance);
  }

  // --- 5. 核心功能：50cm 觸發切換螢幕 ---
  if (currentDistance < 50 && currentDistance > 2) {
    // 檢查是否為新觸發，且距離上次觸發超過 5 秒 (避免切換太頻繁)
    if (!pcIsTriggered && (millis() - lastTriggerTime > 5000)) {
      Serial.println(">>> BOSS COMING! 切換螢幕...");
      
      // 在 TFT 顯示醒目提示
      tft.fillRect(0, 25, 160, 15, ST7735_RED);
      tft.setCursor(5, 28);
      tft.setTextColor(ST7735_WHITE);
      tft.print("Warning!");

      HTTPClient http;
      http.begin("http://192.168.4.2:5000/trigger"); 
      http.setTimeout(1500); 
      int httpCode = http.GET();
      
      if (httpCode > 0) {
        Serial.printf("發送成功: %d\n", httpCode);
      } else {
        Serial.printf("發送失敗: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      
      pcIsTriggered = true;
      lastTriggerTime = millis();
    }
  } else if (currentDistance > 60) {
    // 當距離拉開到 60cm 以上才重置觸發狀態 (防止邊界抖動)
    if (pcIsTriggered) {
      tft.fillRect(0, 25, 160, 15, ST7735_BLACK); // 清除警告文字
      pcIsTriggered = false;
    }
  }

  // 6. 系統處理
  server.handleClient();
  if (autoScan) {
    angle += step;
    if (angle <= 0 || angle >= 180) step = -step;
  } else {
    angle = manualAngle;
  }
  delay(5); 
}