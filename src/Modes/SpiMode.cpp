#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include "SpiMode.h"
#include "config.h"
#include "../CommandLine.h"

int SpiMode::currentMosi = PIN_SPI_MOSI;
int SpiMode::currentMiso = PIN_SPI_MISO;
int SpiMode::currentSck = PIN_SPI_SCK;
int SpiMode::currentCs = PIN_SPI_CS;
int SpiMode::currentClockSpeed = 1000000;

static void printHexByte(uint8_t b) {
  if (b < 0x10) Serial.print("0");
  Serial.print(b, HEX);
}

static void cmdConfig() {
  Serial.print("MOSI pin (current GPIO");
  Serial.print(SpiMode::currentMosi);
  Serial.print("): ");
  int mosi = CommandLine::readInt(0, 39);
  if (mosi == -1) return;

  Serial.print("MISO pin (current GPIO");
  Serial.print(SpiMode::currentMiso);
  Serial.print("): ");
  int miso = CommandLine::readInt(0, 39);
  if (miso == -1) return;

  Serial.print("SCK pin (current GPIO");
  Serial.print(SpiMode::currentSck);
  Serial.print("): ");
  int sck = CommandLine::readInt(0, 39);
  if (sck == -1) return;

  Serial.print("CS pin (current GPIO");
  Serial.print(SpiMode::currentCs);
  Serial.print("): ");
  int cs = CommandLine::readInt(0, 39);
  if (cs == -1) return;

  Serial.print("Clock speed (100000-40000000, current ");
  Serial.print(SpiMode::currentClockSpeed);
  Serial.println("): ");
  int speed = CommandLine::readInt(100000, 40000000);
  if (speed == -1) return;

  SpiMode::currentMosi = mosi;
  SpiMode::currentMiso = miso;
  SpiMode::currentSck = sck;
  SpiMode::currentCs = cs;
  SpiMode::currentClockSpeed = speed;

  SPI.end();
  SPI.begin(sck, miso, mosi, cs);

  Serial.print(F("Config applied for this session. Type 'save' to make it permanent.\nMOSI=GPIO"));
  Serial.print(SpiMode::currentMosi);
  Serial.print(F(", MISO=GPIO"));
  Serial.print(SpiMode::currentMiso);
  Serial.print(F(", SCK=GPIO"));
  Serial.print(SpiMode::currentSck);
  Serial.print(F(", CS=GPIO"));
  Serial.print(SpiMode::currentCs);
  Serial.print(F(", Clock="));
  Serial.println(SpiMode::currentClockSpeed);
}

static void cmdSave() {
  Preferences prefs;
  prefs.begin("nexus_spi", false);
  prefs.putInt("mosi", SpiMode::currentMosi);
  prefs.putInt("miso", SpiMode::currentMiso);
  prefs.putInt("sck", SpiMode::currentSck);
  prefs.putInt("cs", SpiMode::currentCs);
  prefs.putInt("clk", SpiMode::currentClockSpeed);
  prefs.end();
  Serial.print(F("Config saved to NVS: MOSI=GPIO"));
  Serial.print(SpiMode::currentMosi);
  Serial.print(F(", MISO=GPIO"));
  Serial.print(SpiMode::currentMiso);
  Serial.print(F(", SCK=GPIO"));
  Serial.print(SpiMode::currentSck);
  Serial.print(F(", CS=GPIO"));
  Serial.print(SpiMode::currentCs);
  Serial.print(F(", Clock="));
  Serial.println(SpiMode::currentClockSpeed);
}

static volatile uint8_t sniffMosiAccum = 0;
static volatile uint8_t sniffMisoAccum = 0;
static volatile int sniffBitCount = 0;
static volatile bool sniffByteReady = false;
static volatile uint8_t sniffCapturedMosi = 0;
static volatile uint8_t sniffCapturedMiso = 0;
static volatile bool sniffTransactionActive = false;
static volatile bool sniffStartDetected = false;
static volatile bool sniffStopDetected = false;

static void IRAM_ATTR sniffSckRising() {
  if (!sniffTransactionActive) return;
  int mosi = digitalRead(SpiMode::currentMosi) ? 1 : 0;
  int miso = digitalRead(SpiMode::currentMiso) ? 1 : 0;
  sniffMosiAccum = (sniffMosiAccum << 1) | mosi;
  sniffMisoAccum = (sniffMisoAccum << 1) | miso;
  sniffBitCount = sniffBitCount + 1;
  if (sniffBitCount == 8) {
    sniffCapturedMosi = sniffMosiAccum;
    sniffCapturedMiso = sniffMisoAccum;
    sniffByteReady = true;
    sniffBitCount = 0;
    sniffMosiAccum = 0;
    sniffMisoAccum = 0;
  }
}

static void IRAM_ATTR sniffCsChange() {
  int cs = digitalRead(SpiMode::currentCs);
  if (cs == LOW) {
    sniffTransactionActive = true;
    sniffStartDetected = true;
    sniffBitCount = 0;
    sniffMosiAccum = 0;
    sniffMisoAccum = 0;
  } else {
    sniffTransactionActive = false;
    sniffStopDetected = true;
    sniffBitCount = 0;
    sniffMosiAccum = 0;
    sniffMisoAccum = 0;
  }
}

static void cmdSniff() {
  Serial.print(F("SPI Sniffing on MOSI=GPIO"));
  Serial.print(SpiMode::currentMosi);
  Serial.print(F(", MISO=GPIO"));
  Serial.print(SpiMode::currentMiso);
  Serial.print(F(", SCK=GPIO"));
  Serial.print(SpiMode::currentSck);
  Serial.print(F(", CS=GPIO"));
  Serial.println(SpiMode::currentCs);
  Serial.println(F("NOTE: Passive SPI sniffing on ESP32 may be unreliable at higher speeds."));

  SPI.end();

  pinMode(SpiMode::currentMosi, INPUT);
  pinMode(SpiMode::currentMiso, INPUT);
  pinMode(SpiMode::currentSck, INPUT);
  pinMode(SpiMode::currentCs, INPUT);

  sniffTransactionActive = false;
  sniffStartDetected = false;
  sniffStopDetected = false;
  sniffByteReady = false;
  sniffBitCount = 0;
  sniffMosiAccum = 0;
  sniffMisoAccum = 0;

  attachInterrupt(digitalPinToInterrupt(SpiMode::currentSck), sniffSckRising, RISING);
  attachInterrupt(digitalPinToInterrupt(SpiMode::currentCs), sniffCsChange, CHANGE);

  Serial.println(F("╔══════════════════════════════════════════════╗"));
  Serial.println(F("║ Waiting for SPI traffic...                   ║"));
  Serial.println(F("╚══════════════════════════════════════════════╝"));

  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }
    if (sniffStartDetected) {
      Serial.println(F("[START]"));
      sniffStartDetected = false;
    }
    if (sniffStopDetected) {
      Serial.println(F("[STOP]"));
      sniffStopDetected = false;
    }
    if (sniffByteReady) {
      sniffByteReady = false;
      Serial.print(F("MOSI: 0x"));
      printHexByte(sniffCapturedMosi);
      Serial.print(F("  MISO: 0x"));
      printHexByte(sniffCapturedMiso);
      Serial.println();
    }
    delay(1);
  }

  detachInterrupt(digitalPinToInterrupt(SpiMode::currentSck));
  detachInterrupt(digitalPinToInterrupt(SpiMode::currentCs));
  SPI.begin(SpiMode::currentSck, SpiMode::currentMiso, SpiMode::currentMosi, SpiMode::currentCs);
}

static void printSpiHelp() {
  Serial.println(F("╔══════════════╦════════════════════════════════════╗"));
  Serial.println(F("║           NEXUS - SPI                             ║"));
  Serial.println(F("╠══════════════╬════════════════════════════════════╣"));
  Serial.println(F("║ config       ║ Configure SPI pins and speed       ║"));
  Serial.println(F("║ save         ║ Save config to NVS (persistent)    ║"));
  Serial.println(F("║ sniff        ║ Sniff SPI bus traffic              ║"));
  Serial.println(F("╚══════════════╩════════════════════════════════════╝"));
}

void SpiMode::begin() {
  Preferences prefs;
  prefs.begin("nexus_spi", false);
  if (prefs.isKey("mosi")) {
    currentMosi = prefs.getInt("mosi", PIN_SPI_MOSI);
    currentMiso = prefs.getInt("miso", PIN_SPI_MISO);
    currentSck = prefs.getInt("sck", PIN_SPI_SCK);
    currentCs = prefs.getInt("cs", PIN_SPI_CS);
    currentClockSpeed = prefs.getInt("clk", 1000000);
  }
  prefs.end();
  SPI.begin(currentSck, currentMiso, currentMosi, currentCs);
}

void SpiMode::handle(const String& cmd) {
  String command = cmd;
  command.trim();

  if (command == "help") {
    printSpiHelp();
  } else if (command == "config") {
    cmdConfig();
  } else if (command == "save") {
    cmdSave();
  } else if (command == "sniff") {
    cmdSniff();
  } else {
    String upper = command;
    upper.toUpperCase();
    Serial.print("[SPI] ");
    Serial.print(upper);
    Serial.println(" \xe2\x80\x94 not implemented yet.");
  }
}
