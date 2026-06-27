#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include "I2cMode.h"
#include "config.h"
#include "../CommandLine.h"

static int currentSda = PIN_I2C_SDA;
static int currentScl = PIN_I2C_SCL;
static int currentSpeed = 100000;

static int readHex8() {
  while (true) {
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.trim();
    if (line.startsWith("0x") || line.startsWith("0X")) {
      line = line.substring(2);
    }
    bool ok = line.length() > 0 && line.length() <= 2;
    if (ok) {
      for (int i = 0; i < (int)line.length(); i++) {
        if (!isHexadecimalDigit(line[i])) { ok = false; break; }
      }
    }
    if (ok) {
      int val = strtol(line.c_str(), nullptr, 16);
      if (val >= 0 && val <= 0x7F) return val;
    }
    Serial.print("Enter hex (0x00-0x7F): ");
  }
}

static int readHexByte() {
  while (true) {
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.trim();
    if (line.startsWith("0x") || line.startsWith("0X")) {
      line = line.substring(2);
    }
    bool ok = line.length() > 0 && line.length() <= 2;
    if (ok) {
      for (int i = 0; i < (int)line.length(); i++) {
        if (!isHexadecimalDigit(line[i])) { ok = false; break; }
      }
    }
    if (ok) {
      int val = strtol(line.c_str(), nullptr, 16);
      if (val >= 0 && val <= 0xFF) return val;
    }
    Serial.print("Enter hex (0x00-0xFF): ");
  }
}

static int readSpeed() {
  while (true) {
    Serial.print("Clock speed (100000/400000/1000000): ");
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.trim();
    bool ok = line.length() > 0;
    for (int i = 0; i < (int)line.length(); i++) {
      if (!isDigit(line[i])) { ok = false; break; }
    }
    if (ok) {
      int val = line.toInt();
      if (val == 100000 || val == 400000 || val == 1000000) return val;
    }
    Serial.println(F("Valid: 100000, 400000, 1000000"));
  }
}

static void printHexByte(uint8_t b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
}

static void cmdScan() {
  Serial.println(F("Scanning I2C bus..."));
  Serial.println(F("╔════════════╦══════════════════╗"));
  Serial.println(F("║ Address    ║ Device           ║"));
  Serial.println(F("╠════════════╬══════════════════╣"));
  int found = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      const char* devName = "";
      switch (addr) {
        case 0x68: devName = "MPU6050 (IMU)"; break;
        case 0x76: case 0x77: devName = "BMP280 (P/T)"; break;
        case 0x3C: case 0x3D: devName = "SSD1306 (OLED)"; break;
        case 0x27: case 0x3F: devName = "PCF8574 (LCD)"; break;
        case 0x40: devName = "PCA9685 (PWM)"; break;
        case 0x5A: devName = "MLX90614 (IR T)"; break;
        default:
          if (addr >= 0x50 && addr <= 0x57) {
            devName = "24xx EEPROM";
          } else {
            devName = "Unknown";
          }
          break;
      }
      Serial.print(F("║ 0x"));
      printHexByte(addr);
      Serial.print(F("       ║ "));
      Serial.print(devName);
      int pad = 17 - strlen(devName);
      for (int i = 0; i < pad; i++) Serial.print(' ');
      Serial.println(F("║"));
      found++;
    }
    delay(1);
  }
  if (found == 0) {
    Serial.println(F("║ (none found)                  ║"));
  }
  Serial.println(F("╚════════════╩══════════════════╝"));
}

static void cmdConfig() {
  Serial.print("SDA pin (current GPIO");
  Serial.print(currentSda);
  Serial.print("): ");
  int sda = CommandLine::readInt(0, 39);
  if (sda == -1) return;

  Serial.print("SCL pin (current GPIO");
  Serial.print(currentScl);
  Serial.print("): ");
  int scl = CommandLine::readInt(0, 39);
  if (scl == -1) return;

  Serial.print("Clock speed (current ");
  Serial.print(currentSpeed);
  Serial.println(")");
  int speed = readSpeed();
  if (speed == -1) return;

  currentSda = sda;
  currentScl = scl;
  currentSpeed = speed;
  Wire.end();
  Wire.begin(currentSda, currentScl, currentSpeed);

  Serial.print(F("Config applied for this session. Type 'save' to make it permanent.\nSDA=GPIO"));
  Serial.print(currentSda);
  Serial.print(F(", SCL=GPIO"));
  Serial.print(currentScl);
  Serial.print(F(", Speed="));
  Serial.println(currentSpeed);
}

static void cmdSave() {
  Preferences prefs;
  prefs.begin("nexus_i2c", false);
  prefs.putInt("sda", currentSda);
  prefs.putInt("scl", currentScl);
  prefs.putInt("speed", currentSpeed);
  prefs.end();
  Serial.print(F("Config saved to NVS: SDA=GPIO"));
  Serial.print(currentSda);
  Serial.print(F(", SCL=GPIO"));
  Serial.print(currentScl);
  Serial.print(F(", Speed="));
  Serial.println(currentSpeed);
}

static void cmdRead() {
  Serial.print("Device address (hex): ");
  int devAddr = readHex8();
  if (devAddr == -1) return;

  Serial.print("Register address (hex): ");
  int regAddr = readHexByte();
  if (regAddr == -1) return;

  Serial.print("Number of bytes: ");
  int numBytes = CommandLine::readInt(1, 32);
  if (numBytes == -1) return;

  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.print(F("I2C error: "));
    Serial.println(error);
    return;
  }

  delay(10);
  Wire.requestFrom(devAddr, numBytes);
  Serial.print(F("Read: "));
  for (int i = 0; i < numBytes; i++) {
    if (Wire.available()) {
      uint8_t b = Wire.read();
      Serial.print("0x");
      printHexByte(b);
      Serial.print(" ");
    }
  }
  Serial.println();
}

static void cmdWrite() {
  Serial.print("Device address (hex): ");
  int devAddr = readHex8();
  if (devAddr == -1) return;

  Serial.print("Register address (hex): ");
  int regAddr = readHexByte();
  if (regAddr == -1) return;

  Serial.print("Byte value (hex): ");
  int val = readHexByte();
  if (val == -1) return;

  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.write(val);
  uint8_t error = Wire.endTransmission();

  if (error == 0) {
    Serial.print(F("Wrote 0x"));
    printHexByte(val);
    Serial.print(F(" to reg 0x"));
    printHexByte(regAddr);
    Serial.print(F(" at device 0x"));
    printHexByte(devAddr);
    Serial.println();
  } else {
    Serial.print(F("I2C error: "));
    Serial.println(error);
  }
}

static void cmdDump() {
  Serial.print("Device address (hex): ");
  int devAddr = readHex8();
  if (devAddr == -1) return;

  Serial.println();
  Serial.println(F("╔════════════════════════════════════════════════════════════════════════════╗"));
  Serial.print(F("║ I2C DUMP - 0x"));
  printHexByte(devAddr);
  for (int i = 0; i < 60; i++) Serial.print(" ");
  Serial.println(F("║"));
  Serial.println(F("╠════════════════════════════════════════════════════════════════════════════╣"));

  char ascii[17];
  for (int row = 0; row < 16; row++) {
    int base = row * 16;
    Serial.print(F("║ "));
    if (base < 0x10) Serial.print("0");
    Serial.print(base, HEX);
    Serial.print(F(": "));

    for (int col = 0; col < 16; col++) {
      int reg = base + col;
      Wire.beginTransmission(devAddr);
      Wire.write(reg);
      uint8_t err = Wire.endTransmission();
      if (err == 0) {
        Wire.requestFrom(devAddr, (uint8_t)1);
        if (Wire.available()) {
          uint8_t b = Wire.read();
          printHexByte(b);
          Serial.print(" ");
          ascii[col] = (b >= 32 && b <= 126) ? b : '.';
        } else {
          Serial.print(F("-- "));
          ascii[col] = '.';
        }
      } else {
        Serial.print(F("-- "));
        ascii[col] = '.';
      }
      delay(1);
    }
    ascii[16] = '\0';
    Serial.print(F("    ║ "));
    Serial.print(ascii);
    Serial.println(F(" ║"));
  }
  Serial.println(F("╚════════════════════════════════════════════════════════════════════════════╝"));
}

static void cmdMonitor() {
  Serial.print("Device address (hex): ");
  int devAddr = readHex8();
  if (devAddr == -1) return;

  Serial.print("Register address (hex): ");
  int regAddr = readHexByte();
  if (regAddr == -1) return;

  Serial.print("Poll interval (ms): ");
  int interval = CommandLine::readInt(10, 60000);
  if (interval == -1) return;

  Serial.print(F("Monitoring reg 0x"));
  printHexByte(regAddr);
  Serial.print(F(" at dev 0x"));
  printHexByte(devAddr);
  Serial.println(F(". Press any key to stop."));

  int lastVal = -1;
  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }
    Wire.beginTransmission(devAddr);
    Wire.write(regAddr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Wire.requestFrom(devAddr, (uint8_t)1);
      if (Wire.available()) {
        int val = Wire.read();
        if (val != lastVal) {
          Serial.print(millis());
          Serial.print(F(" ms - reg 0x"));
          printHexByte(regAddr);
          Serial.print(F(": 0x"));
          printHexByte(val);
          Serial.print(F(" ("));
          Serial.print(val);
          Serial.println(F(")"));
          lastVal = val;
        }
      }
    }
    delay(interval);
  }
}

static void cmdRecover() {
  Serial.println(F("I2C Bus Recovery:"));
  Serial.print(F("SCL=GPIO"));
  Serial.print(currentScl);
  Serial.print(F(" (OUTPUT), SDA=GPIO"));
  Serial.print(currentSda);
  Serial.println(F(" (INPUT)"));

  pinMode(currentScl, OUTPUT);
  pinMode(currentSda, INPUT);
  digitalWrite(currentScl, HIGH);
  delayMicroseconds(10);

  bool recovered = false;
  for (int i = 0; i < 9; i++) {
    digitalWrite(currentScl, LOW);
    delayMicroseconds(10);
    digitalWrite(currentScl, HIGH);
    delayMicroseconds(10);

    int sdaState = digitalRead(currentSda);
    Serial.print(F("  Cycle "));
    Serial.print(i + 1);
    Serial.print(F(": SCL toggled, SDA="));
    Serial.println(sdaState == HIGH ? "HIGH" : "LOW");

    if (sdaState == HIGH && i >= 1) {
      recovered = true;
      break;
    }
  }

  pinMode(currentScl, INPUT);
  pinMode(currentSda, INPUT);
  Wire.begin(currentSda, currentScl, currentSpeed);

  if (recovered) {
    Serial.println(F("Result: I2C bus recovered successfully."));
  } else {
    Serial.println(F("Result: Recovery attempted. Verify bus connections and pull-up resistors."));
  }
}

static volatile uint8_t sniffByte = 0;
static volatile int sniffBitCount = 0;
static volatile bool sniffByteReady = false;
static volatile uint8_t sniffCapturedByte = 0;
static volatile bool sniffCapturedAck = false;
static volatile bool sniffStartDetected = false;
static volatile bool sniffStopDetected = false;
static int sniffSdaPin = -1;
static int sniffSclPin = -1;

static void IRAM_ATTR sniffSdaChange() {
  int sda = digitalRead(sniffSdaPin);
  int scl = digitalRead(sniffSclPin);
  if (scl == HIGH) {
    if (sda == LOW && !sniffStartDetected) {
      sniffStartDetected = true;
      sniffStopDetected = false;
      sniffByte = 0;
      sniffBitCount = 0;
      sniffByteReady = false;
    } else if (sda == HIGH && sniffStartDetected) {
      sniffStopDetected = true;
      sniffStartDetected = false;
      sniffByte = 0;
      sniffBitCount = 0;
      sniffByteReady = false;
    }
  }
}

static void IRAM_ATTR sniffSclRising() {
  if (!sniffStartDetected || sniffStopDetected) return;
  int sda = digitalRead(sniffSdaPin);
  if (sniffBitCount < 8) {
    sniffByte = (sniffByte << 1) | (sda ? 1 : 0);
    sniffBitCount = sniffBitCount + 1;
  } else {
    sniffCapturedByte = sniffByte;
    sniffCapturedAck = (sda == 0);
    sniffByteReady = true;
    sniffBitCount = 0;
    sniffByte = 0;
  }
}

static void cmdSniff() {
  sniffSdaPin = currentSda;
  sniffSclPin = currentScl;

  Serial.print(F("I2C Sniffing on SDA=GPIO"));
  Serial.print(sniffSdaPin);
  Serial.print(F(", SCL=GPIO"));
  Serial.print(sniffSclPin);
  Serial.println(F(" (INPUT). Press any key to stop."));
  Serial.println(F("NOTE: Passive sniffing on ESP32 may be unreliable at higher speeds."));

  pinMode(sniffSdaPin, INPUT);
  pinMode(sniffSclPin, INPUT);

  sniffStartDetected = false;
  sniffStopDetected = false;
  sniffByte = 0;
  sniffBitCount = 0;
  sniffByteReady = false;

  attachInterrupt(digitalPinToInterrupt(sniffSdaPin), sniffSdaChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(sniffSclPin), sniffSclRising, RISING);

  Serial.println(F("╔══════════════════════════════════════════════╗"));
  Serial.println(F("║ Waiting for I2C traffic...                   ║"));
  Serial.println(F("╚══════════════════════════════════════════════╝"));

  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }
    if (sniffStopDetected) {
      Serial.println(F("[STOP]"));
      sniffStopDetected = false;
    }
    if (sniffByteReady) {
      sniffByteReady = false;
      Serial.print(F("0x"));
      printHexByte(sniffCapturedByte);
      if (sniffCapturedAck) {
        Serial.print(F("[ACK] "));
      } else {
        Serial.print(F("[NACK] "));
      }
    }
    delay(1);
 }

  detachInterrupt(digitalPinToInterrupt(sniffSdaPin));
  detachInterrupt(digitalPinToInterrupt(sniffSclPin));
  Wire.begin(currentSda, currentScl, currentSpeed);
}

static void printI2cHelp() {
  Serial.println(F("╔══════════════╦════════════════════════════════════╗"));
  Serial.println(F("║           NEXUS - I2C                             ║"));
  Serial.println(F("╠══════════════╬════════════════════════════════════╣"));
  Serial.println(F("║ scan         ║ Scan I2C bus for devices           ║"));
  Serial.println(F("║ config       ║ Configure I2C pins and speed       ║"));
  Serial.println(F("║ save         ║ Save config to NVS (persistent)    ║"));
  Serial.println(F("║ read         ║ Read from device register          ║"));
  Serial.println(F("║ write        ║ Write to device register           ║"));
  Serial.println(F("║ dump         ║ Dump all registers (hex grid)      ║"));
  Serial.println(F("║ monitor      ║ Monitor register for changes       ║"));
  Serial.println(F("║ recover      ║ Recover stuck I2C bus              ║"));
  Serial.println(F("║ sniff        ║ Sniff I2C bus traffic              ║"));
  Serial.println(F("╚══════════════╩════════════════════════════════════╝"));
}

void I2cMode::begin() {
  Preferences prefs;
  prefs.begin("nexus_i2c", false);
  if (prefs.isKey("sda")) {
    currentSda = prefs.getInt("sda", PIN_I2C_SDA);
    currentScl = prefs.getInt("scl", PIN_I2C_SCL);
    currentSpeed = prefs.getInt("speed", 100000);
  }
  prefs.end();
  Wire.begin(currentSda, currentScl, currentSpeed);
}

void I2cMode::handle(const String& cmd) {
  String command = cmd;
  command.trim();

  if (command == "help") {
    printI2cHelp();
  } else if (command == "scan") {
    cmdScan();
  } else if (command == "config") {
    cmdConfig();
  } else if (command == "save") {
    cmdSave();
  } else if (command == "read") {
    cmdRead();
  } else if (command == "write") {
    cmdWrite();
  } else if (command == "dump") {
    cmdDump();
  } else if (command == "monitor") {
    cmdMonitor();
  } else if (command == "recover") {
    cmdRecover();
  } else if (command == "sniff") {
    cmdSniff();
  } else {
    String upper = command;
    upper.toUpperCase();
    Serial.print("[I2C] ");
    Serial.print(upper);
    Serial.println(" — not implemented yet.");
  }
}
