#include <Arduino.h>
#include "CommandLine.h"
#include "Modes/DioMode.h"
#include "Modes/I2cMode.h"
#include "Modes/SpiMode.h"
#include "Modes/UartMode.h"

String CommandLine::currentMode = "";
String CommandLine::currentShell = "";

static String inputBuffer;
static unsigned long inputStartTime = 0;

void CommandLine::flushSerialInput() {
  while (Serial.available()) {
    Serial.read();
    delay(1);
  }
  unsigned long start = millis();
  while (millis() - start < 50) {
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      start = millis();
    }
    delay(1);
  }
}

String CommandLine::readInput() {
  String line;
  while (true) {
    while (!Serial.available()) { delay(10); }
    char c = Serial.read();
    if (c == '\n') {
      Serial.println();
      return line;
    } else if (c == '\r') {
      // ignore
    } else if (c == '\b' || c == 127) {
      if (line.length() > 0) {
        line.remove(line.length() - 1);
        Serial.print(F("\b \b"));
      }
    } else if (c >= 32 && c <= 126) {
      line += c;
      Serial.print(c);
    }
  }
}

bool CommandLine::isAbortCommand(const String& s) {
  String t = s;
  t.toLowerCase();
  t.trim();
  return t == "exit" || t == "cancel" || t == "abort";
}

int CommandLine::readInt(int minVal, int maxVal) {
  while (true) {
    String line = readInput();
    if (isAbortCommand(line)) return -1;
    line.trim();
    bool ok = line.length() > 0;
    for (int i = 0; i < (int)line.length(); i++) {
      if (!isDigit(line[i])) { ok = false; break; }
    }
    if (ok) {
      int val = line.toInt();
      if (val >= minVal && val <= maxVal) return val;
    }
    Serial.print("Enter ");
    Serial.print(minVal);
    Serial.print("-");
    Serial.print(maxVal);
    Serial.print(": ");
  }
}

static void printPrompt() {
  CommandLine::flushSerialInput();
  if (CommandLine::currentShell.length() > 0) {
    Serial.print("nexus/");
    Serial.print(CommandLine::currentMode);
    Serial.print("/");
    Serial.print(CommandLine::currentShell);
    Serial.print("> ");
  } else if (CommandLine::currentMode.length() > 0) {
    Serial.print("nexus/");
    Serial.print(CommandLine::currentMode);
    Serial.print("> ");
  } else {
    Serial.print("nexus> ");
  }
}

static void printGlobalHelp() {
  Serial.println(F("╔══════════════╦═══════════════════════════════╗"));
  Serial.println(F("║            NEXUS - GLOBAL                    ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║ uart         ║ UART protocol tools           ║"));
  Serial.println(F("║ i2c          ║ I2C protocol tools            ║"));
  Serial.println(F("║ spi          ║ SPI protocol tools            ║"));
  Serial.println(F("║ dio          ║ Digital I/O tools             ║"));
  Serial.println(F("╚══════════════╩═══════════════════════════════╝"));
  Serial.println(F("Type: mode <name>  (e.g. mode dio)"));
}

static bool isValidMode(const String& name) {
  return name == "uart" || name == "i2c" || name == "spi" ||
         name == "dio";
}

static void dispatchCommand(const String& cmd) {
  int spaceIdx = cmd.indexOf(' ');
  String command = (spaceIdx == -1) ? cmd : cmd.substring(0, spaceIdx);
  String args = (spaceIdx == -1) ? "" : cmd.substring(spaceIdx + 1);
  args.trim();
  command.toLowerCase();

  if (command == "help") {
    if (CommandLine::currentMode.length() > 0) {
      if (CommandLine::currentMode == "dio") {
        DioMode::handle("help");
      } else if (CommandLine::currentMode == "i2c") {
        I2cMode::handle("help");
      } else if (CommandLine::currentMode == "spi") {
        SpiMode::handle("help");
      } else if (CommandLine::currentMode == "uart") {
        UartMode::handle("help");
      }
    } else {
      printGlobalHelp();
    }
  } else if (command == "exit") {
    if (CommandLine::currentShell.length() > 0) {
      CommandLine::currentShell = "";
    } else if (CommandLine::currentMode.length() > 0) {
      CommandLine::currentMode = "";
    } else {
      Serial.println(F("Already at root. Use Ctrl+C to exit the serial monitor."));
    }
  } else if (command == "mode") {
    if (args.length() == 0) {
      Serial.println(F("Usage: mode <name>"));
    } else {
      args.toLowerCase();
      if (isValidMode(args)) {
        CommandLine::currentMode = args;
        CommandLine::currentShell = "";
      } else {
        Serial.print(F("Invalid mode: "));
        Serial.print(args);
        Serial.println(F(". Valid modes: uart, i2c, spi, dio"));
      }
    }
  } else {
    if (CommandLine::currentMode.length() > 0) {
      if (CommandLine::currentMode == "dio") {
        DioMode::handle(command);
      } else if (CommandLine::currentMode == "i2c") {
        I2cMode::handle(command);
      } else if (CommandLine::currentMode == "spi") {
        SpiMode::handle(command);
      } else if (CommandLine::currentMode == "uart") {
        UartMode::handle(command);
      } else {
        String modeUpper = CommandLine::currentMode;
        modeUpper.toUpperCase();
        Serial.print("[");
        Serial.print(modeUpper);
        Serial.print("] ");
        Serial.print(cmd);
        Serial.println(" \xe2\x80\x94 not implemented yet.");
      }
    } else if (CommandLine::currentShell.length() > 0) {
      String shellUpper = CommandLine::currentShell;
      shellUpper.toUpperCase();
      Serial.print("[");
      Serial.print(shellUpper);
      Serial.print("] ");
      Serial.print(cmd);
      Serial.println(" \xe2\x80\x94 not implemented yet.");
    } else {
      Serial.print("Unknown command: ");
      Serial.print(cmd);
      Serial.println(F(". Type 'help' for options."));
    }
  }
}

void CommandLine::begin() {
  printPrompt();
}

void CommandLine::loop() {
  static unsigned long lastByteTime = 0;

  if (inputBuffer.length() == 0 && Serial.available()) {
    if (lastByteTime == 0) {
      lastByteTime = millis();
    } else if (millis() - lastByteTime > 200) {
      while (Serial.available()) Serial.read();
      lastByteTime = 0;
    }
  } else {
    lastByteTime = 0;
  }

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      String line = inputBuffer;
      inputBuffer = String();
      inputStartTime = 0;
      Serial.println();

      line.trim();
      if (line.length() > 0) {
        dispatchCommand(line);
      }
      inputBuffer = String();
      printPrompt();
    } else if (c == '\r') {
      // ignore — \n handles processing
    } else if (c == '\b' || c == 127) {
      if (inputBuffer.length() > 0) {
        inputBuffer.remove(inputBuffer.length() - 1);
        Serial.print(F("\b \b"));
      }
    } else if (c >= 32 && c <= 126) {
      if (inputBuffer.length() == 0) {
        inputStartTime = millis();
      }
      inputBuffer += c;
      Serial.print(c);
    }
  }

  if (inputBuffer.length() > 0 && inputStartTime > 0 &&
      millis() - inputStartTime > 60000) {
    inputBuffer = String();
    inputStartTime = 0;
    Serial.println();
    printPrompt();
  }
}
