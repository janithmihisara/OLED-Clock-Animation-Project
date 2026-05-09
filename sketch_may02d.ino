#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define SWITCH1 A2
#define SWITCH2 A3
#define VIBRATION_MOTOR_PIN 9  // Vibration motor control pin

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

// Eye animation variables
int leftEyeX = 45;
int rightEyeX = 80;
int eyeY = 18;
int eyeWidth = 25;
int eyeHeight = 30;

int targetOffsetX = 0;
int targetOffsetY = 0;
int moveSpeed = 5;

int blinkState = 0;
int blinkDelay = 4000;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

// Heart bitmap (not used in this version)
const unsigned char heartBitmap[] PROGMEM = {
  0b00001100, 0b00110000,
  0b00011110, 0b01111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b01111111, 0b11111110,
  0b00111111, 0b11111100,
  0b00011111, 0b11111000,
  0b00001111, 0b11110000,
  0b00000111, 0b11100000,
  0b00000011, 0b11000000,
  0b00000001, 0b10000000,
  0b00000000, 0b00000000,
};

void setup() {
  pinMode(SWITCH1, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);
  pinMode(VIBRATION_MOTOR_PIN, OUTPUT);
  digitalWrite(VIBRATION_MOTOR_PIN, LOW); // Initially off

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  if (!rtc.begin()) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Couldn't find RTC");
    display.display();
    while (1);
  }
}

void loop() {
  bool s1 = !digitalRead(SWITCH1);
  bool s2 = !digitalRead(SWITCH2);

  if (s1 && !s2) {
    showEyeAnimation();
  } else if (!s1 && !s2) {
    showAnalogClock();
  } else if (s1 && s2) {
    showDigitalClock();
  } else if (!s1 && s2) {
    drawWeatherAnimation();
  }

  delay(50);
}

// ========== MODE FUNCTIONS ==========

void showEyeAnimation() {
  unsigned long currentTime = millis();

  // Blinking logic
  if (currentTime - lastBlinkTime > blinkDelay && blinkState == 0) {
    blinkState = 1;
    lastBlinkTime = currentTime;
  } else if (currentTime - lastBlinkTime > 150 && blinkState == 1) {
    blinkState = 0;
    lastBlinkTime = currentTime;
  }

  // Eye movement logic
  static bool isMoving = false;
  static unsigned long vibrationStart = 0;
  if (currentTime - moveTime > random(1500, 3000) && blinkState == 0) {
    int movementType = random(0, 7);
    if (movementType == 0) { targetOffsetX = -10; targetOffsetY = 0; }
    else if (movementType == 1) { targetOffsetX = 10; targetOffsetY = 0; }
    else if (movementType == 2) { targetOffsetX = -10; targetOffsetY = -8; }
    else if (movementType == 3) { targetOffsetX = 10; targetOffsetY = -8; }
    else if (movementType == 4) { targetOffsetX = -10; targetOffsetY = 8; }
    else if (movementType == 5) { targetOffsetX = 10; targetOffsetY = 8; }
    else { targetOffsetX = 0; targetOffsetY = 0; }

    moveTime = currentTime;
    isMoving = true;
    digitalWrite(VIBRATION_MOTOR_PIN, HIGH);
    vibrationStart = currentTime;
  }

  // Stop vibration after 200ms
  if (isMoving && currentTime - vibrationStart > 200) {
    digitalWrite(VIBRATION_MOTOR_PIN, LOW);
    isMoving = false;
  }

  // Offset calculation for smooth eye movement
  static int offsetX = 0;
  static int offsetY = 0;
  offsetX += (targetOffsetX - offsetX) / moveSpeed;
  offsetY += (targetOffsetY - offsetY) / moveSpeed;

  display.clearDisplay();
  if (blinkState == 0) {
    drawEye(leftEyeX + offsetX, eyeY + offsetY, eyeWidth, eyeHeight);
    drawEye(rightEyeX + offsetX, eyeY + offsetY, eyeWidth, eyeHeight);
  } else {
    display.fillRect(leftEyeX + offsetX, eyeY + offsetY + eyeHeight / 2 - 2, eyeWidth, 4, WHITE);
    display.fillRect(rightEyeX + offsetX, eyeY + offsetY + eyeHeight / 2 - 2, eyeWidth, 4, WHITE);
  }
  display.display();
}

void drawEye(int x, int y, int w, int h) {
  display.fillRoundRect(x, y, w, h, 5, WHITE);
}

void showAnalogClock() {
  DateTime now = rtc.now();
  display.clearDisplay();
  int centerX = 64;
  int centerY = 32;
  display.drawCircle(centerX, centerY, 30, WHITE);
  float angleH = ((now.hour() % 12) + now.minute() / 60.0) * 30;
  float angleM = now.minute() * 6;
  float angleS = now.second() * 6;
  drawHand(centerX, centerY, angleH, 15);
  drawHand(centerX, centerY, angleM, 20);
  drawHand(centerX, centerY, angleS, 25);
  display.display();
}

void drawHand(int x, int y, float angleDeg, int length) {
  float angleRad = radians(angleDeg - 90);
  int xEnd = x + cos(angleRad) * length;
  int yEnd = y + sin(angleRad) * length;
  display.drawLine(x, y, xEnd, yEnd, WHITE);
}

void showDigitalClock() {
  DateTime now = rtc.now();
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  char buffer[10];
  sprintf(buffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display.println(buffer);
  display.display();
}

void drawWeatherAnimation() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  int columnWidth = 6;
  int rowHeight = 8;

  for (int i = 0; i < SCREEN_WIDTH / columnWidth; i++) {
    int value = random(0, 2);
    char binaryChar = (value == 1) ? '1' : '0';
    int startY = random(0, SCREEN_HEIGHT - rowHeight);
    display.setCursor(i * columnWidth, startY);
    display.print(binaryChar);
  }

  display.display();
}
