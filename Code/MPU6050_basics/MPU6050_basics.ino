#include <Wire.h>
#include <math.h>

const int resolution2g = 0x00;          //00000000(0x00) → AFS_SEL = 00 (±2g)
const int resolution4g = 0x08;          //00001000(0x08) → AFS_SEL = 01 (±4g)
const int resolution8g = 0x10;          //00010000(0x10) → AFS_SEL = 10 (±8g)
const int resolution16g = 0x18;         //00011000(0x18) → AFS_SEL = 11 (±16g)
const float factorAcc = 16384.0 * 2.0;  //umrechung pro g Auflösung
const float factorGyro = 131.0;         //Umrechnungsfaktor der Gyrodaten
const int sensor = 0x68;                //Sensoradresse
int resolution = resolution2g;

//Gyrooffset
float gyro_x_offset = 0.0;
float gyro_y_offset = 0.0;

// Filter-Parameter
const float alpha = 0.95;  // Komplementärfilter Gewichtung
float roll_filtered = 0.0;
float pitch_filtered = 0.0;
unsigned long lastTime = 0;

float pitch_offset = 0.0;

//Regler-Parameter
float Kp = 1.0;
float Kd = 0.5;
float Ki = 1.0;
float integral = 0.0;
float control = 0.0;

//Pins
const int motorMax = 255;
const int MOTOR_LEFT_STEP = 2;
const int MOTOR_LEFT_DIR = 5;
const int MOTOR_RIGHT_STEP = 3;
const int MOTOR_RIGHT_DIR = 6;
const int ENABLE_PIN = 8;

unsigned long lastStepTimeLeft = 0;
unsigned long lastStepTimeRight = 0;


//Datenstruktur für Beschleunigungswerte
struct AccelData {
  int16_t ax_raw;
  int16_t ay_raw;
  int16_t az_raw;
};

//Datenstruktur für Gyroskopwerte
struct GyroData {
  int16_t gx_raw;
  int16_t gy_raw;
  int16_t gz_raw;
};

void setup() {
  Serial.begin(115200);
  Wire.begin();
  wakeUp(sensor);   // Sensor aufwecken
  setResolution();  // Auflösung des Sensor einstellen
  calibrateGyro(sensor);
  calibratePitchZero(sensor);

  // MOTOR-PINS
  pinMode(MOTOR_LEFT_STEP, OUTPUT);
  pinMode(MOTOR_LEFT_DIR, OUTPUT);
  pinMode(MOTOR_RIGHT_STEP, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT); 

  // Treiber aktivieren
  digitalWrite(ENABLE_PIN, LOW);

  lastTime = micros();
}

void loop() {

  unsigned long now = micros();
  float dt = (now - lastTime) / 1000000.0;  // in Sekunden
  lastTime = now;

  //Rohdaten auslesen
  AccelData accel = readAccel(sensor);
  GyroData gyro = readGyro(sensor);

  //Rohdaten in "g" Wandeln
  float ax = convertAcc(accel.ax_raw);
  float ay = convertAcc(accel.ay_raw);
  float az = convertAcc(accel.az_raw);

  float gx = convertGyro(gyro.gx_raw - gyro_x_offset);
  float gy = convertGyro(gyro.gy_raw - gyro_y_offset);

  //Beschleunigungswinkel
  float roll_acc = atan(ay / sqrt(ax * ax + az * az)) * 180.0 / PI;
  float pitch_acc = atan(ax / sqrt(ay * ay + az * az)) * 180.0 / PI;

  // Komplementärfilter
  roll_filtered = alpha * (roll_filtered + gy * dt) + (1 - alpha) * roll_acc;
  pitch_filtered = alpha * (pitch_filtered + gx * dt) + (1 - alpha) * pitch_acc;

  float pitch = pitch_filtered - pitch_offset;

  //Regelung
  float error = -pitch;

  // Anti-Windup für Integral
  if (abs(error) < 15) {
    integral += error * dt;
    integral = constrain(integral, -100, 100);
  } else {
    integral = 0;
  }

  integral += error * dt;
  control = (Kp * error) + (Ki * integral) + (Kd * gx);  // P + D;

  control = constrain(control, -motorMax, motorMax);
  float absControl = abs(control);
  int stepDelay = map(absControl, 0, motorMax, 500, 2500);
  //stepDelay = constrain(stepDelay, 400, 1500);
  //stepDelay = max(stepDelay, 300);

  if (absControl < 50) {
    // Motor stoppen, falls fast aufrecht
    integral = 0;
  } else {
    // Motor-Ansteuerung
    if (now - lastStepTimeLeft >= stepDelay) {
      digitalWrite(MOTOR_LEFT_DIR, control >= 0 ? HIGH : LOW);  // Richtung setzen
      digitalWrite(MOTOR_LEFT_STEP, HIGH);
      digitalWrite(MOTOR_LEFT_STEP, LOW);
      lastStepTimeLeft = now;
    }

    //Rechter Motor
    if (now - lastStepTimeRight >= stepDelay) {
      digitalWrite(MOTOR_RIGHT_DIR, control >= 0 ? HIGH : LOW);  // Richtung setzen
      digitalWrite(MOTOR_RIGHT_STEP, HIGH);
      digitalWrite(MOTOR_RIGHT_STEP, LOW);
      lastStepTimeRight = now;
    }
  }





  // Kontrollausgabe
  // Serial.println("---------------------------");
  // Serial.print("AX: "); Serial.print(ax); Serial.print("\t");
  // Serial.print("AY: "); Serial.print(ay); Serial.print("\t");
  // Serial.print("AZ: "); Serial.println(az);
  // Serial.print("Roll: "); Serial.print(roll_filtered); Serial.print("\t");
  // Serial.println("Pitch: "); Serial.println(pitch_filtered);

  // Serial.print("Pitch (null): ");
  // Serial.println(pitch);
  // Serial.print("\tControl: ");
  // Serial.println(control);
  //delay(1000);
}


AccelData readAccel(int sensorAddr) {
  AccelData data;

  Wire.beginTransmission(sensorAddr);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(sensorAddr, 6);  // nur Accelerometer

  data.ax_raw = Wire.read() << 8 | Wire.read();  //Wire.read() liest 8 bits und speichert sie dann in ax (16bit integer) der << (bitsshift links) verschiebt diese bits dann nach links und verodert das mit einem weiteren Wire.read()
  //                                         bsp: az=0000 0000 0000 0000 vor dem lesen
  //                                         az=0000 0000 1010 0010 nach dem ersten Wire.read() nur die hinteren acht bits werden gelesen
  //                                         az=1010 0010 0000 0000 nach dem bitsshift << 8
  //                                         az=1010 0010 0000 0000 | 0000 0000 1111 0011 = 1010 0010 1111 0011 -> sensorwert als 16Bit Integer
  data.ay_raw = Wire.read() << 8 | Wire.read();
  data.az_raw = Wire.read() << 8 | Wire.read();

  return data;
}


//siehe readAccel
GyroData readGyro(int sensorAddr) {
  GyroData data;
  Wire.beginTransmission(sensorAddr);
  Wire.write(0x43);  // GYRO_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(sensorAddr, 6);

  data.gx_raw = Wire.read() << 8 | Wire.read();
  data.gy_raw = Wire.read() << 8 | Wire.read();
  data.gz_raw = Wire.read() << 8 | Wire.read();

  return data;
}


void wakeUp(int sensor) {
  Wire.beginTransmission(sensor);
  Wire.write(0x6B);  // PWR_MGMT_1
  Wire.write(0x00);
  Wire.endTransmission();
  Serial.println("Sensor an Adresse 0x" + String(sensor, HEX) + " aufgeweckt");
}

void setResolution() {
  Wire.beginTransmission(0x68);
  Wire.write(0x1C);  // ACCEL_CONFIG
  Wire.write(resolution);
  Wire.endTransmission();

  Serial.println("Auflösung des Sensors auf ± " + String(pow(2, ((resolution >> 3) + 1))) + "g gesetzt");
}

float convertAcc(int16_t acceleration) {
  return acceleration * (1 << ((resolution >> 3) + 1)) / factorAcc;
}

float convertGyro(int16_t gyro) {
  return gyro / 131.0;  // ±250°/s → 131 LSB/°/s
}

void calibrateGyro(int sensorAddr) {
  const int samples = 1000;
  long sumX = 0;
  long sumY = 0;

  Serial.println("Gyro-Kalibrierung läuft...");
  delay(2000);  // Zeit, um Roboter ruhig hinzustellen

  for (int i = 0; i < samples; i++) {
    GyroData g = readGyro(sensorAddr);
    sumX += g.gx_raw;
    sumY += g.gy_raw;
    delay(2);
  }

  gyro_x_offset = sumX / (float)samples;
  gyro_y_offset = sumY / (float)samples;

  Serial.print("Gyro X Offset: ");
  Serial.println(gyro_x_offset);
  Serial.print("Gyro Y Offset: ");
  Serial.println(gyro_y_offset);
}

void calibratePitchZero(int sensorAddr) {
  const int samples = 200;
  float sum = 0.0;

  Serial.println("Pitch-Nullung...");
  delay(2000);  // Zeit zum ruhig hinstellen

  for (int i = 0; i < samples; i++) {
    unsigned long now = micros();
    float dt = (now - lastTime) / 1000000.0;
    ;
    lastTime = now;

    AccelData accel = readAccel(sensorAddr);
    GyroData gyro = readGyro(sensorAddr);

    float ax = convertAcc(accel.ax_raw);
    float ay = convertAcc(accel.ay_raw);
    float az = convertAcc(accel.az_raw);

    float gx = convertGyro(gyro.gx_raw - gyro_x_offset);

    float pitch_acc = atan(ax / sqrt(ay * ay + az * az)) * 180.0 / PI;

    pitch_filtered = alpha * (pitch_filtered + gx * dt)
                     + (1 - alpha) * pitch_acc;

    if (i > 50) sum += pitch_filtered;
    delay(5);
  }

  pitch_offset = sum / (samples - 50);

  Serial.print("Pitch Offset gesetzt auf: ");
  Serial.println(pitch_offset);
}
