#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#include "intro.h"

#include "pet_animation.h"

//================== กำหนดพินให้แต่ละอุปกรณ์ ได้แก่ (5อุปกรณ์) ==================
  //กำหนดไฟ LED ทั้ง 3 สี
  #define LED_RED 25
  #define LED_YELLOW 26
  #define LED_GREEN 27
  //กำหนดปุ่มกดทั้ง 3 ปุ่ม
  #define BUTTON_L 17
  #define BUTTON_M 18
  #define BUTTON_R 19
  //กำหนดจอ OLED
  #define OLED_SDA 21
  #define OLED_SCL 22
  #define SCREEN_WIDTH 128
  #define SCREEN_HIGHT 64
  #define OLED_RESET -1
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HIGHT, &Wire, OLED_RESET);
  //กำหนดที่วัดอุณหภูมิ DHT11
  #define DHTPIN 15
  #define DHTTYPE DHT11
  DHT dht(DHTPIN, DHTTYPE);
  //กำหนดที่ปรับระดับ PWM
  #define motorPWM 23
  #define freqPWM 5000
  #define resoPWM 8
  //กำหนด Potentiometer (ADC)
  #define POT_PIN 34

//====================== กำหนดค่าและประกาศตัวแปร ======================
  int menuIndex = 0; 
  const int totalMenuItems = 3;
  //กำหนดค่าขอบแกน Y ของแสดงหน้าเครดิต
  int creditYPos = SCREEN_HIGHT;
  //กำหนดค่าของตั้งค่าระบบเวลา
  int setHour = 1;
  int setMinute = 0;
  bool isAM = true;
  int setDay = 1;
  int setMonth = 1;
  int setYear = 2026;
  int settingField = 0;
  int systemState = 0;

  //กำหนดค่าสถานะของสัตว์เลี้ยง
  int petHunger = 100; // 0-100
  int petHappy = 100;  // 0-100
  int petClean = 100;  // 0-100

  //ตัวแปรอ่านอุณหภูมิและสถานะป่วย
  float roomTemp = 25.0;
  bool isSick = false;

  //ตัวแปรสำหรับการคุม UI ในเกม
  bool showActionBar = false; // ตัวแปรเช็คการแสดงผล Status & Menu Bar (false = แสดงแค่ตัวละคร)
  int actionIndex = 0;        // 0: Feed, 1: Clean, 2: Play

  //ตัวแปรในการเช็คเงื่อนไขของสัตว์เลี้ยงตอนโต
  bool strictHungerMaintained = true;
  bool strictHappyMaintained = true; 
  bool strictCleanMaintained = true; 

  unsigned long lastPetUpdate = 0;
  unsigned long stageStateTime = 0; 
  unsigned long zeroStartTime = 0;  
  bool isZeroTime = false;

  bool MiniGame = false;
  int miniGameScore = 0;

//สถานะสัตว์เลี้ยง
enum PetState {
  STATE_PET_EGG,
  STATE_PET_ADULT,
  STATE_PET_DEAD
};
PetState currentPetState = STATE_PET_EGG;

//ประเภทของสัตว์เลี้ยงตอนโต
enum PetType {
  ADULT_NONE,   
  ADULT_HUNGRER, 
  ADULT_HAPPY,  
  ADULT_CLEAN   
};
PetType currentPetType = ADULT_NONE;

//สถานะการแสดงผลทางหน้าจอ UI
enum SystemState {
  STATE_INTRO,
  STATE_MAIN,
  STATE_SETTINGS,
  STATE_CREDITS,
  STATE_GAMEPLAY
};
SystemState currentSystemState = STATE_INTRO;

//แสดงออกทางเจอ monitor บน VScode
void sendTelemetryLog(const char* action) {
  Serial.printf("[TELEMETRY] Action: %s | Hunger: %d | Happy: %d | Clean: %d | Temp: %.1fC | Sick: %s\n",
                action, petHunger, petHappy, petClean, roomTemp, isSick ? "YES" : "NO");
}

//========================= ล้างจอเมื่อเริ่มต้นใหม่ =========================
void FontSetting(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
}

//=========================== ตั้งค่าระบบเริ่มต้น ===========================
void setup(){
  Serial.begin(115200);
  Serial.println("Welcome to LiveTouchTamagotchi...");
  
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  
  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_M, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);
  
  pinMode(POT_PIN, INPUT);

  ledcAttachPin(motorPWM, freqPWM);
  ledcWrite(motorPWM, 0);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)){
    Serial.println("Sorry, display failed...");
    for(;;);
  }
  
  dht.begin();
  FontSetting();
  display.display();
}

void fadeOut(){
  for (int c = 255; c >= 0; c -= 5) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(c);
    delay(7);
  }
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void fadeIn(){
  display.ssd1306_command(SSD1306_DISPLAYON);  
  display.ssd1306_command(SSD1306_SETCONTRAST); 
  display.ssd1306_command(0);

  for (int d = 0; d <= 255; d += 5) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(d);
    delay(7);
  }
}

void fadeBoth(){
  fadeOut();
  delay(500);
  fadeIn();
  delay(500);
}

void Loading(){
  fadeBoth();
  for (int load = 0; load < 3; load++){
    display.clearDisplay();
    display.setCursor(34, 28);
    display.print(F("Loading."));
    display.display();
    delay(400);

    display.clearDisplay();
    display.setCursor(34, 28);
    display.print(F("Loading.."));
    display.display();
    delay(400);

    display.clearDisplay();
    display.setCursor(34, 28);
    display.print(F("Loading..."));
    display.display();
    delay(400);
  }
  delay(500);
  fadeBoth();
}

//=========================== ตั้งค่าระบบเวลา ===========================
void TimeModify(int val){
  switch(settingField){
    case 0:
      setHour += val;
      if(setHour > 12) setHour = 1;
      if(setHour < 1) setHour = 12;
      break;
    case 1:
      setMinute += val;
      if(setMinute > 60) setMinute = 1;
      if(setMinute < 1) setMinute = 60;
      break;
    case 2:
      isAM = !isAM;
      break;
    case 3:
      setDay += val;
      if(setDay > 31) setDay = 1;
      if(setDay < 1) setDay = 31;
      break;
    case 4:
      setMonth += val;
      if(setMonth > 12) setMonth = 1;
      if(setMonth < 1) setMonth = 12;
      break;
    case 5:
      setYear += val;
      if(setYear > 2046) setYear = 2026;
      if(setYear < 2026) setYear = 2046;
      break;
  }
}

void TimeSetting(){
  if(digitalRead(BUTTON_M) == LOW){
    settingField++;
    if(settingField > 5){
      currentSystemState = STATE_MAIN;
    }
    delay(300);
  }

  if(digitalRead(BUTTON_L) == LOW){
    TimeModify(1);
    delay(150);
  }
  if(digitalRead(BUTTON_R) == LOW){
    TimeModify(-1);
    delay(150);
  }

  FontSetting();
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);
  display.setCursor(30, 3);
  display.print(F("TIME SETTING"));
  
  display.setCursor(5, 20);
  display.print(F("Time: "));
  if(settingField == 0) display.print(F("["));
  if(setHour < 10) display.print("0");
  display.print(setHour);
  if(settingField == 0) display.print(F("]"));
  display.print(" : ");
  
  if(settingField == 1) display.print(F("["));
  if(setMinute < 10) display.print("0");
  display.print(setMinute);
  if(settingField == 1) display.print(F("]"));
  display.print(" ");
  
  if(settingField == 2) display.print(F("["));
  display.print(isAM ? "AM" : "PM");
  if(settingField == 2) display.print(F("]"));
  
  display.setCursor(5, 35);
  display.print(F("Date: "));
  
  if(settingField == 3) display.print(F("["));
  if(setDay < 10) display.print("0");
  display.print(setDay);
  if(settingField == 3) display.print(F("]"));
  display.print("/");
  
  if(settingField == 4) display.print(F("["));
  if(setMonth < 10) display.print("0");
  display.print(setMonth);
  if(settingField == 4) display.print(F("]"));
  display.print("/");
  
  if(settingField == 5) display.print(F("["));
  display.print(setYear);
  if(settingField == 5) display.print(F("]"));

  if (settingField <= 5){
    display.drawLine(0, 47, 128, 47, SSD1306_WHITE);
    display.setCursor(5, 52);
    display.print(F("+"));
    display.setCursor(60, 52);
    display.print(F("OK"));
    display.setCursor(120, 52);
    display.print(F("-"));
  }

  display.display();
}

//=========================== แสดงหน้าเครดิต ===========================
void ShowCredit(){
  display.clearDisplay();
  display.setTextSize(1);

  int startY = creditYPos;

  display.setCursor(13, startY+0);
  display.print(F("-Hardware Systems-"));
  display.setCursor(25, startY+15);
  display.print(F("Siriwit Samlit"));
  display.setCursor(40, startY+40);
  display.print(F("-Coding-"));
  display.setCursor(5, startY+55);
  display.print(F("Kannika Phakkaraphon"));
  display.setCursor(20, startY+80);
  display.print(F("-Animation & UI-"));
  display.setCursor(42, startY+95);
  display.print(F("Tanisorn"));
  display.setCursor(27, startY+110);
  display.print(F("Ngamjunyaporn"));
  display.setCursor(25, startY+130);
  display.print(F("--------------"));
  display.setCursor(35, startY+140);
  display.print(F("THANK YOU!"));
  display.setCursor(25, startY+150);
  display.print(F("--------------"));
  
  display.fillRect(0, 0, 128, 20, SSD1306_BLACK);
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);
  display.setCursor(43, 3);
  display.print(F("CREDITS"));
  display.display();

  creditYPos--;
  if (creditYPos < -140) {
    creditYPos = SCREEN_HIGHT;
  }
  delay(40);
  
  if(digitalRead(BUTTON_M) == LOW){
    display.clearDisplay();
    display.setCursor(10, 28);
    display.print(F("Exiting Credits..."));
    display.display();
    delay(1000);
    currentSystemState = STATE_MAIN;
  }
}

//========================== อนิเมชั่นสัตว์เลี้ยง ==========================
bool Intro_animation(){
  fadeIn();
  delay(500);

  // 2. ลูปเล่นอนิเมชั่นจนกว่า handleIntroScreen() จะคืนค่า true (เล่นจบครบ 64 เฟรม)
  while (!handleIntroScreen()) {
    delay(50); // ความเร็วในการเล่นแต่ละเฟรม (ปรับตัวเลขตามความเหมาะสม)
  }

  // 3. แสดงเฟรมสุดท้ายค้างไว้ครู่หนึ่งก่อนดับ
  delay(500);

  fadeOut();
  delay(500);
  fadeIn();
  return true;
}
//=========================== ระบบสัตว์เลี้ยง ===========================
void resetGame(){
  currentPetState = STATE_PET_EGG;
  currentPetType = ADULT_NONE;
  petHunger = 100;
  petHappy = 100;
  petClean = 100;
  strictHungerMaintained = true;
  strictHappyMaintained = true;
  strictCleanMaintained = true;
  stageStateTime = millis();
  isZeroTime = false;
  MiniGame = false;
  showActionBar = false;
  actionIndex = 0;
}

void updatePetState(){
  if (currentPetState == STATE_PET_DEAD) return;

  if(millis() - lastPetUpdate > 30000){
    if(petHunger > 0) petHunger -= 1;
    if(petHappy > 0) petHappy -= 5;
    if(petClean > 0) petClean -= 1;

    float t = dht.readTemperature();
    if(!isnan(t)){
      roomTemp = t;
      if(roomTemp > 33.0) isSick = true;
      else if(roomTemp <= 30.0 && isSick) isSick = false;
    }

    if(petHunger < 50) strictHungerMaintained = false;
    if(petHappy < 50) strictHappyMaintained = false;
    if(petClean < 50) strictCleanMaintained = false;

    if(petHunger <= 0 || petHappy <= 0 || petClean <= 0){
      if(!isZeroTime){
        isZeroTime = true;
        zeroStartTime = millis();
      }
      else{
        if(millis() - zeroStartTime > 60000) currentPetState = STATE_PET_DEAD;
      }
    }
    else{
      isZeroTime = false;
    }

    if(isSick || petHunger < 20 || petHappy < 20) {
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
    } else {
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
    }

    sendTelemetryLog("PERIODIC_DECAY");
    lastPetUpdate = millis();
  }

  if(currentPetState == STATE_PET_EGG){
    if(millis() - stageStateTime > 60000){
      currentPetState = STATE_PET_ADULT;
      if(strictHungerMaintained) currentPetType = ADULT_HUNGRER;
      else if(strictHappyMaintained) currentPetType = ADULT_HAPPY;
      else if(strictCleanMaintained) currentPetType = ADULT_CLEAN;
      else currentPetType = ADULT_CLEAN;
    }
  }
}

void UIshow(){
  display.clearDisplay();
  display.setTextSize(1);

  // 1. วาดสถานะ Game Over
  if(currentPetState == STATE_PET_DEAD){
    display.setTextSize(2);
    display.setCursor(10, 15);
    display.print(F("GAME OVER"));
    display.setTextSize(1);
    display.setCursor(10, 45);
    display.print(F("Pet Died! Resetting..."));
    display.display();
    return;
  }

  // 2. วาดตัวละคร / อนิเมชัน
  if (currentPetState == STATE_PET_EGG) {
    display.setTextSize(2);
    display.setCursor(48, 25);
    display.print(F("(@)"));
    
    if(showActionBar){
      unsigned long elapsed = millis() - stageStateTime;
      long remainingTime = 60000 - elapsed;
      if (remainingTime < 0) remainingTime = 0;
      int remSec = (remainingTime / 1000) % 60;
      int remMin = (remainingTime / 1000) / 60;
      
      display.setTextSize(1);
      display.setCursor(20, 38);
      display.print(F("Hatch: "));
      if (remMin < 10) display.print("0");
      display.print(remMin);
      display.print(":");
      if (remSec < 10) display.print("0");
      display.print(remSec);
    }
  }
  else if (currentPetState == STATE_PET_ADULT) {
    // วาดอนิเมชันลงใน Buffer
    if (currentPetType == ADULT_HUNGRER) { AnimationPet_hunger(display); }
    else if (currentPetType == ADULT_HAPPY) { AnimationPet_happy(display); }
    else if (currentPetType == ADULT_CLEAN) { AnimationPet_clean(display); }

    if(isSick){
      display.setTextSize(1);
      display.setCursor(95, 25);
      display.print(F("HOT!"));
    }
  }

  // 3. วาด UI / Status Bar ทับด้านบนและด้านล่าง (ถ้าเปิด showActionBar)
  if(showActionBar){
    // แถบบน
    display.fillRect(0, 0, 128, 13, SSD1306_BLACK);
    display.setCursor(2, 2);
    display.print(F("H:")); display.print(petHunger); display.print(F("%"));
    display.setCursor(48, 2);
    display.print(F("C:")); display.print(petClean); display.print(F("%"));
    display.setCursor(92, 2);
    display.print(F("P:")); display.print(petHappy); display.print(F("%"));
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // แถบล่าง
    display.fillRect(0, 48, 128, 16, SSD1306_BLACK);
    display.drawLine(0, 48, 128, 48, SSD1306_WHITE);
    
    display.setCursor(4, 54);
    if(actionIndex == 0) display.print(F("[Feed]"));
    else display.print(F(" Feed "));

    display.setCursor(48, 54);
    if(actionIndex == 1) display.print(F("[Clean]"));
    else display.print(F(" Clean "));

    display.setCursor(92, 54);
    if(actionIndex == 2) display.print(F("[Play]"));
    else display.print(F(" Play "));
  }

  // 4. ส่งข้อมูลภาพขึ้นจอ OLED พร้อมกันทีเดียว
  display.display();
}

//=========================== มินิเกมทายทิศทาง ===========================
void runMiniGame(){
  int targetDirection = 0;
  int playerChoice = -1;
  miniGameScore = 0;
  //เคลียร์สถานะปุ่ม รอให้ผู้เล่นปล่อยปุ่มทั้งหมดก่อนเริ่มเกม
  while(digitalRead(BUTTON_L) == LOW || digitalRead(BUTTON_M) == LOW || digitalRead(BUTTON_R) == LOW) {
    delay(10);
  }
  delay(200);
  //นับรอบทั้งหมด 3 รอบ
  for (int round = 1; round <= 3; round++) {
    targetDirection = random(0, 3); // 0=L, 1=M, 2=R
    unsigned long roundStartTime = millis();
    playerChoice = -1;
    bool answered = false;
    //มีเวลาให้ผู้เล่นกดภายใน 5 วินาที
    while (millis() - roundStartTime < 5000) {
      unsigned long elapsed = millis() - roundStartTime;
      long remainingTime = 5000 - (long)elapsed;
      if (remainingTime < 0) remainingTime = 0;
      int remSec = (remainingTime / 1000) + 1;
      //วาง header
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 2);
      display.print(F("== GUESS DIRECTION =="));
      //วางรอบไว้ footer
      display.setCursor(35, 52);
      display.print(F("Round ")); 
      display.print(round); 
      display.print(F("/3"));
      //วาง UI ของระบบทับถอยหลัง
      display.setCursor(62, 16);
      display.print(remSec);
      //สุ่มสัญลักษณ์ < , ^ , >
      display.setTextSize(2);
      display.setCursor(58, 30);
      if (targetDirection == 0) display.print(F("<"));
      else if (targetDirection == 1) display.print(F("^"));
      else display.print(F(">"));
      display.display();
      //ถ้ามันตรงกับสัญลักษณ์ ให้ถูก
      if (digitalRead(BUTTON_L) == LOW) { playerChoice = 0; answered = true; break; }
      if (digitalRead(BUTTON_M) == LOW) { playerChoice = 1; answered = true; break; }
      if (digitalRead(BUTTON_R) == LOW) { playerChoice = 2; answered = true; break; }
      delay(10);
    }
    //แสดงผลลัพธ์บนจอ OLED และไฟ LED ประจำรอบ
    display.setTextSize(1);
    display.fillRect(0,10,128,40,SSD1306_BLACK);
    //กรณีหมดเวลา
    if (!answered){
      display.setCursor(40, 25);
      display.print(F("TIME OUT!"));
      digitalWrite(LED_YELLOW, HIGH);
      delay(1000);
    }
    //กรณีตอบถูก
    else if (playerChoice == targetDirection){
      miniGameScore++;
      display.setCursor(40, 25);
      display.print(F("CORRECT!"));
      digitalWrite(LED_GREEN, HIGH);
      delay(1000);
    }
    //กรณีตอบผิด
    else{
      display.setCursor(45, 25);
      display.print(F("WRONG!"));
      digitalWrite(LED_RED, HIGH);
      delay(1000);
    }
    display.display();
    delay(1000);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    delay(400);
  }
  //เมื่อเล่นจบ จะนำจำนวนที่ตอบถูกมาคำนวนเป็นค่าความสุข (คูณ10)
  int addedHappy = (miniGameScore * 10); 
  if (addedHappy < 5) addedHappy = 5;
  petHappy += addedHappy;
  if (petHappy > 100) petHappy = 100;
  //Mini Game จบลง
  MiniGame = false;
  fadeBoth();
}

//=========================== แสดงการเล่นเกม ===========================
void handleGameScreen() {
  updatePetState();
  //กรณีสัตว์เลี้ยงตาย
  if(currentPetState == STATE_PET_DEAD){
    UIshow();
    digitalWrite(LED_RED, HIGH);
    //รอจนกว่าจะกดปุ่ม M
    while(digitalRead(BUTTON_M) != LOW) {
      delay(10);
    }
    digitalWrite(LED_RED, LOW);
    resetGame();
    currentSystemState = STATE_MAIN;
    delay(200);
    return;
  }
  //แสดงผล UI
  UIshow();
  
  //หน้าจอตัวละครเดี่ยว (showActionBar == false)
  if(!showActionBar) {
    //กดปุ่ม M เพื่อเปิดแถบ Status & Menu
    if (digitalRead(BUTTON_M) == LOW) {
      showActionBar = true;
      actionIndex = 0; // เริ่มต้นเลือกที่ Feed
      delay(200);
    }
  } 
  //หน้าจอขณะเปิดแถบ Status & Menu (showActionBar == true)
  else {
    //ปุ่ม L เลื่อนตัวเลือกไปทางขวา
    if (digitalRead(BUTTON_L) == LOW) {
      actionIndex++;
      if (actionIndex > 2) actionIndex = 0;
      delay(200);
    }
    //ปุ่ม R ย้อนกลับไปหน้าตัวละครเดี่ยว
    if (digitalRead(BUTTON_R) == LOW) {
      showActionBar = false;
      delay(200);
    }
    //ปุ่ม M ตกลงทำคำสั่งที่เลือกไว้
    if (digitalRead(BUTTON_M) == LOW) {
      //เลือก Feed
      if (actionIndex == 0) {
        int potVal = analogRead(POT_PIN); // 0 - 4095
        int foodAmount = map(potVal, 0, 4095, 10, 35); // แปลงเป็นปริมาณอาหาร 10 - 35
        
        petHunger += foodAmount;
        if (petHunger > 100) petHunger = 100;

        digitalWrite(LED_GREEN, HIGH);
        delay(200);
        digitalWrite(LED_GREEN, LOW);

        sendTelemetryLog("FEED_ACTION");
      } 
      else if (actionIndex == 1) {
        petClean = 100;
        digitalWrite(LED_YELLOW, HIGH);
        delay(200);
        digitalWrite(LED_YELLOW, LOW);

        sendTelemetryLog("CLEAN_ACTION");
      } 
      else if (actionIndex == 2) {
        MiniGame = true;
        runMiniGame();
      }
      delay(200);
    }
  }
}

//=========================== แสดงหน้าจอเมนู =========================== 
void MainMenu(){
  FontSetting();
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);
  display.setCursor(50, 3);
  display.print(F("MENU"));
  
  display.setCursor(27, 25); display.print(F("Start Game"));
  display.setCursor(27, 35); display.print(F("Settings"));
  display.setCursor(27, 45); display.print(F("Credits"));

  for(int i = 0; i < 3; i++){
    display.setCursor(15, 25 + (i * 10));
    if(i == menuIndex) display.print(F("> "));
    else display.print(F(" "));
  }
  display.display();

  if(digitalRead(BUTTON_L) == LOW){
    menuIndex++;
    if(menuIndex >= totalMenuItems) menuIndex = 0;
    delay(200);
  }

  if(digitalRead(BUTTON_R) == LOW){
    menuIndex--;
    if(menuIndex < 0) menuIndex = totalMenuItems - 1;
    delay(200);
  }

  if(digitalRead(BUTTON_M) == LOW){
    if(menuIndex == 0) {
      currentSystemState = STATE_GAMEPLAY; 
      Loading(); 
      showActionBar = false; // เมื่อเข้าเกม เริ่มต้นที่หน้าตัวละครเดี่ยวเสมอ
      UIshow(); 
      fadeIn();
    }
    else if(menuIndex == 1){ currentSystemState = STATE_SETTINGS; settingField = 0; }
    else if(menuIndex == 2){ currentSystemState = STATE_CREDITS; creditYPos = SCREEN_HIGHT; }
    delay(200);
  }
}

//=============================== Loop ===============================
void loop(){
  switch (currentSystemState) {
    case STATE_INTRO:
      if (Intro_animation()) {
        currentSystemState = STATE_MAIN; // main.cpp จัดการเปลี่ยน state เอง
      }
      break;
    case STATE_MAIN:
      MainMenu();
      break;
    case STATE_GAMEPLAY:
      handleGameScreen();
      break;
    case STATE_SETTINGS:
      TimeSetting();
      break;
    case STATE_CREDITS:
      ShowCredit();
      break;
  }
  delay(50);
}