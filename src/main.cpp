#include <Arduino.h>
#include "config.h"
#include "CommandLine.h"
#include "Modes/I2cMode.h"
#include "Modes/SpiMode.h"
#include "Modes/UartMode.h"

void setup() {
  Serial.begin(BAUD_RATE);
  delay(100);
  while (Serial.available()) Serial.read();

  Serial.println();
  Serial.println(F("╔═════════════════════════════════════════════╗"));
  Serial.println(F("║  ███╗   ██╗███████╗██╗  ██╗██╗   ██╗███████╗║"));
  Serial.println(F("║  ████╗  ██║██╔════╝╚██╗██╔╝██║   ██║██╔════╝║"));
  Serial.println(F("║  ██╔██╗ ██║█████╗   ╚███╔╝ ██║   ██║███████╗║"));
  Serial.println(F("║  ██║╚██╗██║██╔══╝   ██╔██╗ ██║   ██║╚════██║║"));
  Serial.println(F("║  ██║ ╚████║███████╗██╔╝ ██╗╚██████╔╝███████║║"));
  Serial.println(F("║  ╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝║"));
  Serial.println(F("║           v1.0 | ESP32 | Serial CLI         ║"));
  Serial.println(F("╚═════════════════════════════════════════════╝"));
  Serial.println();

  UartMode::begin();
  I2cMode::begin();
  SpiMode::begin();

  CommandLine::begin();
}

void loop() {
  CommandLine::loop();
}
