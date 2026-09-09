#include <PZEM004Tv30.h>

// ---------------- UART PINS ----------------
#define RXD2 16
#define TXD2 17

// ---------------- OUTPUT PINS ----------------
#define RELAY_PIN 26
#define BUZZER_PIN 27

// ---------------- PASSWORD ----------------
String PASSWORD = "CHANGE_THIS_PASSWORD";

// ---------------- THRESHOLDS ----------------
#define VOLTAGE_PRESENT 180.0
#define MIN_CURRENT 0.05
#define MAX_CURRENT 5.0
#define DROP_PERCENT 40.0
#define VERIFY_TIME 5000

// ---------------- OBJECT ----------------
PZEM004Tv30 pzem(Serial2, RXD2, TXD2);

// ---------------- VARIABLES ----------------
float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;

float lastCurrent = 0;

unsigned long bypassTimer = 0;
unsigned long overloadTimer = 0;

bool tamperLock = false;

// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("=================================");
  Serial.println("SMART ENERGY METER STARTED");
  Serial.println("Tamper Detection Active");
  Serial.println("=================================");
}

// ---------------- LOOP ----------------
void loop()
{
  readSensor();
  displayReadings();

  if (!tamperLock)
  {
    detectMaintenance();
    detectFullBypass();
    detectPartialBypass();
    detectOverload();
  }
  else
  {
    checkPasswordReset();
  }

  lastCurrent = current;

  delay(2000);
}

// ---------------- READ SENSOR ----------------
void readSensor()
{
  voltage = pzem.voltage();
  current = pzem.current();
  power   = pzem.power();
  energy  = pzem.energy();

  if (isnan(voltage) || isnan(current))
  {
    Serial.println("Sensor error");
    return;
  }
}

// ---------------- DISPLAY ----------------
void displayReadings()
{
  Serial.print("Voltage: ");
  Serial.print(voltage);
  Serial.print(" V  Current: ");
  Serial.print(current);
  Serial.print(" A  Power: ");
  Serial.print(power);
  Serial.print(" W  Energy: ");
  Serial.println(energy);
}

// ---------------- MAINTENANCE DETECTION ----------------
void detectMaintenance()
{
  if (voltage < 50)
  {
    Serial.println("Power OFF / Maintenance detected");
    resetTimers();
  }
}

// ---------------- FULL BYPASS ----------------
void detectFullBypass()
{
  if (voltage > VOLTAGE_PRESENT && current < MIN_CURRENT)
  {
    if (bypassTimer == 0)
      bypassTimer = millis();

    if (millis() - bypassTimer > VERIFY_TIME)
    {
      triggerTamper("FULL BYPASS DETECTED");
    }
  }
  else
  {
    bypassTimer = 0;
  }
}

// ---------------- PARTIAL BYPASS ----------------
void detectPartialBypass()
{
  if (lastCurrent > 0.5)
  {
    float drop = ((lastCurrent - current) / lastCurrent) * 100;

    if (drop > DROP_PERCENT && voltage > VOLTAGE_PRESENT)
    {
      triggerTamper("PARTIAL BYPASS DETECTED");
    }
  }
}

// ---------------- OVERLOAD ----------------
void detectOverload()
{
  if (current > MAX_CURRENT)
  {
    if (overloadTimer == 0)
      overloadTimer = millis();

    if (millis() - overloadTimer > VERIFY_TIME)
    {
      triggerTamper("OVERLOAD DETECTED");
    }
  }
  else
  {
    overloadTimer = 0;
  }
}

// ---------------- TRIGGER TAMPER ----------------
void triggerTamper(const char* reason)
{
  Serial.println("=================================");
  Serial.print("ALERT: ");
  Serial.println(reason);
  Serial.println("POWER DISCONNECTED");
  Serial.println("Enter password to restore power");
  Serial.println("=================================");

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);

  tamperLock = true;
}

// ---------------- PASSWORD RESET ----------------
void checkPasswordReset()
{
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == PASSWORD)
    {
      Serial.println("ACCESS GRANTED");
      Serial.println("POWER RESTORED");

      tamperLock = false;

      digitalWrite(RELAY_PIN, HIGH);
      digitalWrite(BUZZER_PIN, LOW);

      resetTimers();
    }
    else
    {
      Serial.println("WRONG PASSWORD");
    }
  }
}

// ---------------- RESET TIMERS ----------------
void resetTimers()
{
  bypassTimer = 0;
  overloadTimer = 0;
}