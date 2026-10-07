#ifndef PET_ANIMATION_H
#define PET_ANIMATION_H

#include <Adafruit_SSD1306.h>

//ประกาศฟังก์ชันอนิเมชันให้ไฟล์อื่นนำไปใช้ได้
void AnimationPet_clean(Adafruit_SSD1306 &display);
void AnimationPet_hunger(Adafruit_SSD1306 &display);
void AnimationPet_happy(Adafruit_SSD1306 &display);

#endif