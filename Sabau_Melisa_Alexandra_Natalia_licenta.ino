#include <ESP32Servo.h>

#define INPUT_PIN 34
#define SERVO_PIN 18


#define SERVO_MIN 0
#define SERVO_MAX 180
#define SERVO_CENTER 90
#define STEP_ANGLE 20

#define INVERT_SERVO_DIRECTION true

Servo servo;
int currentAngle = SERVO_CENTER;


float offset = 0;
float filtered = 0;
float alpha = 0.2;


#define RIGHT_THRESHOLD -600
#define LEFT_THRESHOLD   600
#define NEUTRAL_ZONE     300


int blinkThreshold = 2600;     
int blinkRelease   = 2300;     

bool blinkDetected = false;
unsigned long lastBlink = 0;
const unsigned long blinkDebounce = 500;

int blinkCount = 0;
unsigned long firstBlinkTime = 0;
const unsigned long doubleBlinkWindow = 1600;



bool controlMode = false;
bool waitingNeutral = false;



unsigned long lastSample = 0;
unsigned long lastPrint = 0;
unsigned long lastCommandTime = 0;
unsigned long startTime = 0;

#define SAMPLE_TIME 5
#define PRINT_TIME 50

const unsigned long START_IGNORE_TIME = 6000;
const unsigned long COMMAND_DELAY = 1200;



unsigned long blockBlinkUntil = 0;
const unsigned long BLINK_BLOCK_AFTER_DIRECTION = 2500;



void setup() {

  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  servo.attach(SERVO_PIN, 500, 2400);

  currentAngle = SERVO_CENTER;
  servo.write(currentAngle);

  Serial.println("Servo pozitionat la centru...");
  delay(2000);

  Serial.println("Calibrare... priveste in fata");

  offset = 0;

  for (int i = 0; i < 300; i++) {
    offset += analogRead(INPUT_PIN);
    delay(5);
  }

  offset /= 300.0;

  filtered = 0;

  Serial.print("Offset = ");
  Serial.println(offset);

  startTime = millis();

  Serial.println("Sistem pornit.");
  Serial.println("Dupa 6 secunde, fa DOUA CLIPITURI pentru activare.");
}


void loop() {

  unsigned long now = millis();

  int raw = analogRead(INPUT_PIN);

  bool blinkEventThisLoop = false;

 

  if (now - lastSample >= SAMPLE_TIME) {

    lastSample = now;

    float signal = raw - offset;

    filtered = alpha * signal + (1 - alpha) * filtered;

    if (now - startTime > START_IGNORE_TIME && now > blockBlinkUntil) {
      blinkEventThisLoop = detectDoubleBlink(raw, now);
    }
  }



  if (now - lastPrint >= PRINT_TIME) {

  lastPrint = now;

  
  Serial.print(filtered);
  Serial.print(",");

  Serial.print("Zero:");
  Serial.print(0);
  Serial.print(",");
  Serial.print("Angle=");
  Serial.print(currentAngle);
  Serial.println(controlMode ? "ACTIV" : "INACTIV");
}

  
  if (now - startTime < START_IGNORE_TIME)
    return;

  
  if (blinkEventThisLoop)
    return;

 
  if (!controlMode)
    return;

  
  if (waitingNeutral) {

    if (abs(filtered) < NEUTRAL_ZONE) {
      waitingNeutral = false;
      Serial.println("READY");
    }

    return;
  }



  if (now - lastCommandTime < COMMAND_DELAY)
    return;

  

  if (filtered < RIGHT_THRESHOLD) {

    moveServoRight();

    Serial.print("COMANDA DREAPTA -> Unghi servo = ");
    Serial.println(currentAngle);

    afterDirectionCommand(now);
    return;
  }



  if (filtered > LEFT_THRESHOLD) {

    moveServoLeft();

    Serial.print("COMANDA STANGA -> Unghi servo = ");
    Serial.println(currentAngle);

    afterDirectionCommand(now);
    return;
  }
}



bool detectDoubleBlink(int raw, unsigned long now) {

  bool blinkEvent = false;

 

  if (raw > blinkThreshold &&
      !blinkDetected &&
      now - lastBlink > blinkDebounce) {

    blinkDetected = true;
    blinkEvent = true;
    lastBlink = now;

    Serial.println("CLIPIT DETECTAT");

  
    waitingNeutral = true;
    lastCommandTime = now;

    if (blinkCount == 0) {
      blinkCount = 1;
      firstBlinkTime = now;
    }
    else {
      if (now - firstBlinkTime <= doubleBlinkWindow) {
        blinkCount++;
      } else {
        blinkCount = 1;
        firstBlinkTime = now;
      }
    }

    Serial.print("Numar clipiri = ");
    Serial.println(blinkCount);

    if (blinkCount >= 2) {

      controlMode = !controlMode;

      blinkCount = 0;
      waitingNeutral = true;
      lastCommandTime = now;

      if (controlMode) {
        Serial.println("=== MOD CONTROL ACTIVAT ===");
      } else {
        Serial.println("=== MOD CONTROL OPRIT ===");
      }
    }
  }

  if (raw < blinkRelease) {
    blinkDetected = false;
  }

  if (blinkCount == 1 && now - firstBlinkTime > doubleBlinkWindow) {
    blinkCount = 0;
  }

  return blinkEvent;
}


void moveServoRight() {

  if (INVERT_SERVO_DIRECTION) {
    currentAngle -= STEP_ANGLE;
  } else {
    currentAngle += STEP_ANGLE;
  }

  currentAngle = constrain(currentAngle, SERVO_MIN, SERVO_MAX);
  servo.write(currentAngle);
}

void moveServoLeft() {

  if (INVERT_SERVO_DIRECTION) {
    currentAngle += STEP_ANGLE;
  } else {
    currentAngle -= STEP_ANGLE;
  }

  currentAngle = constrain(currentAngle, SERVO_MIN, SERVO_MAX);
  servo.write(currentAngle);
}

void afterDirectionCommand(unsigned long now) {

  lastCommandTime = now;
  waitingNeutral = true;


  blockBlinkUntil = now + BLINK_BLOCK_AFTER_DIRECTION;

  blinkDetected = false;
  blinkCount = 0;
}