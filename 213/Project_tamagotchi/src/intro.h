
#ifndef INTRO_H
#define INTRO_H

#include <Adafruit_SSD1306.h>

// บอกคอมไพเลอร์ว่าตัวแปร display ถูกสร้างไว้อีกไฟล์หนึ่งแล้ว
extern Adafruit_SSD1306 display;

void initDisplay();
bool handleIntroScreen();

#endif