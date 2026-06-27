#include <Arduino.h>
#include <Preferences.h>
#include "UartMode.h"
#include "../config.h"
#include "../CommandLine.h"

HardwareSerial UartBus(2);

static int currentTx = PIN_UART_TX;
static int currentRx = PIN_UART_RX;
static int currentBaud = 9600;
static int currentParity = 0;
static int currentStopBits = 1;

static uint32_t getSerialConfig(int parity, int stopBits) {
  if (parity == 0) return (stopBits == 2) ? SERIAL_8N2 : SERIAL_8N1;
  if (parity == 1) return (stopBits == 2) ? SERIAL_8E2 : SERIAL_8E1;
  return (stopBits == 2) ? SERIAL_8O2 : SERIAL_8O1;
}

static const char* parityLabel(int p) {
  return (p == 0) ? "N" : (p == 1) ? "E" : "O";
}

static void applyUart() {
  UartBus.end();
  UartBus.setRxBufferSize(1024);
  UartBus.begin(currentBaud, getSerialConfig(currentParity, currentStopBits), currentRx, currentTx);
}

static void printUartHelp() {
  Serial.println(F("╔══════════════╦════════════════════════════════════╗"));
  Serial.println(F("║           NEXUS - UART                            ║"));
  Serial.println(F("╠══════════════╬════════════════════════════════════╣"));
  Serial.println(F("║ config       ║ Configure UART pins, baud, parity  ║"));
  Serial.println(F("║ save         ║ Save config to NVS (persistent)    ║"));
  Serial.println(F("║ scan         ║ Scan baud rates for printable data ║"));
  Serial.println(F("║ bridge       ║ Bidirectional UART bridge          ║"));
  Serial.println(F("║ read         ║ Read bytes from UART               ║"));
  Serial.println(F("║ write        ║ Write data to UART                 ║"));
  Serial.println(F("║ ping         ║ Ping UART device                   ║"));

  Serial.println(F("╚══════════════╩════════════════════════════════════╝"));
}

static void printCurrentConfig() {
  Serial.print(F("TX=GPIO"));
  Serial.print(currentTx);
  Serial.print(F(", RX=GPIO"));
  Serial.print(currentRx);
  Serial.print(F(", "));
  Serial.print(currentBaud);
  Serial.print(F(" 8"));
  Serial.print(parityLabel(currentParity));
  Serial.println(currentStopBits);
}

static int readBaud() {
  while (true) {
    Serial.print(F("Baud rate (300-115200, common: 9600, 19200, 38400, 57600, 115200): "));
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.trim();
    bool ok = line.length() > 0;
    for (int i = 0; i < (int)line.length(); i++) {
      if (!isDigit(line[i])) { ok = false; break; }
    }
    if (ok) {
      int val = line.toInt();
      if (val >= 300 && val <= 115200) return val;
    }
  }
}

static int readParity() {
  while (true) {
    Serial.print(F("Parity (NONE/EVEN/ODD): "));
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.toUpperCase();
    line.trim();
    if (line == "NONE" || line == "N") return 0;
    if (line == "EVEN" || line == "E") return 1;
    if (line == "ODD" || line == "O") return 2;
    Serial.println(F("Valid: NONE, EVEN, ODD"));
  }
}

static void cmdConfig() {
  Serial.print(F("TX pin (current GPIO"));
  Serial.print(currentTx);
  Serial.print(F("): "));
  int tx = CommandLine::readInt(0, 39);
  if (tx == -1) return;

  Serial.print(F("RX pin (current GPIO"));
  Serial.print(currentRx);
  Serial.print(F("): "));
  int rx = CommandLine::readInt(0, 39);
  if (rx == -1) return;

  Serial.print(F("Baud rate (current "));
  Serial.print(currentBaud);
  Serial.println(F(")"));
  int baud = readBaud();
  if (baud == -1) return;

  int parity = readParity();
  if (parity == -1) return;

  Serial.print(F("Stop bits (current "));
  Serial.print(currentStopBits);
  Serial.print(F("): "));
  int stop = CommandLine::readInt(1, 2);
  if (stop == -1) return;

  currentTx = tx;
  currentRx = rx;
  currentBaud = baud;
  currentParity = parity;
  currentStopBits = stop;
  applyUart();

  Serial.print(F("Config applied for this session. Type 'save' to make it permanent.\n"));
  printCurrentConfig();
}

static void cmdSave() {
  Preferences prefs;
  prefs.begin("nexus_uart", false);
  prefs.putInt("tx", currentTx);
  prefs.putInt("rx", currentRx);
  prefs.putInt("baud", currentBaud);
  prefs.putInt("parity", currentParity);
  prefs.putInt("stop", currentStopBits);
  prefs.end();
  Serial.print(F("Config saved to NVS: "));
  printCurrentConfig();
}

struct BaudTestResult {
  bool hasData;
  int printablePct;
  int totalBytes;
};

static BaudTestResult testBaudRate(int baud) {
  UartBus.end();
  UartBus.begin(baud, SERIAL_8N1, currentRx, currentTx);
  delay(100);
  while (UartBus.available()) UartBus.read();

  int printableBytes = 0;
  int totalBytes = 0;
  unsigned long start = millis();
  while (millis() - start < 1500) {
    if (UartBus.available()) {
      uint8_t b = UartBus.read();
      totalBytes++;
      if ((b >= 32 && b <= 126) || b == '\n' || b == '\r' || b == '\t') {
        printableBytes++;
      }
    }
    delay(1);
  }

  BaudTestResult result;
  result.totalBytes = totalBytes;
  result.hasData = totalBytes > 0;
  result.printablePct = totalBytes > 0 ? (printableBytes * 100) / totalBytes : 0;
  return result;
}

static void cmdScan() {
  int savedTx = currentTx;
  int savedRx = currentRx;
  int savedBaud = currentBaud;
  int savedParity = currentParity;
  int savedStop = currentStopBits;
  uint32_t savedConf = getSerialConfig(savedParity, savedStop);

  int rates[] = {9600, 19200, 38400, 57600, 115200, 4800, 2400};
  int numRates = 7;
  BaudTestResult results[7];

  Serial.println(F("Scanning baud rates..."));
  for (int i = 0; i < numRates; i++) {
    results[i] = testBaudRate(rates[i]);
  }

  UartBus.end();
  UartBus.begin(savedBaud, savedConf, savedRx, savedTx);
  currentTx = savedTx;
  currentRx = savedRx;
  currentBaud = savedBaud;
  currentParity = savedParity;
  currentStopBits = savedStop;

  const int C1 = 14, C2 = 16, C3 = 16;

  Serial.println(F("╔══════════════╦════════════════╦════════════════╗"));
  Serial.print(F("║ "));
  Serial.print(F("Baud Rate"));
  for (int p = 9; p < C1 - 1; p++) Serial.print(' ');
  Serial.print(F("║ "));
  Serial.print(F("Data Received"));
  for (int p = 13; p < C2 - 1; p++) Serial.print(' ');
  Serial.print(F("║ "));
  Serial.print(F("Printable %"));
  for (int p = 11; p < C3 - 1; p++) Serial.print(' ');
  Serial.println(F("║"));

  Serial.println(F("╠══════════════╬════════════════╬════════════════╣"));
  for (int i = 0; i < numRates; i++) {
    Serial.print(F("║ "));
    Serial.print(rates[i]);
    for (int p = String(rates[i]).length(); p < C1 - 1; p++) Serial.print(' ');
    Serial.print(F("║ "));

    if (results[i].hasData) {
      Serial.print(F("Yes"));
      for (int p = 3; p < C2 - 1; p++) Serial.print(' ');
      Serial.print(F("║ "));
      Serial.print(results[i].printablePct);
      Serial.print('%');
      for (int p = String(results[i].printablePct).length() + 1; p < C3 - 1; p++) Serial.print(' ');
      Serial.println(F("║"));
    } else {
      Serial.print(F("No"));
      for (int p = 2; p < C2 - 1; p++) Serial.print(' ');
      Serial.print(F("║ N/A"));
      for (int p = 3; p < C3 - 1; p++) Serial.print(' ');
      Serial.println(F("║"));
    }
  }
  Serial.println(F("╚══════════════╩════════════════╩════════════════╝"));
}

static void cmdBridge() {
  Serial.println(F("Entering bridge mode. Type ~exit to quit."));
  while (UartBus.available()) UartBus.read();

  String recent = "";

  while (true) {
    while (Serial.available()) {
      char c = Serial.read();
      UartBus.write(c);
      recent += c;
      if (recent.length() > 10) recent.remove(0, recent.length() - 10);
      if (recent.endsWith("~exit\n") || recent.endsWith("~exit\r")) {
        Serial.println(F("Exiting bridge mode."));
        return;
      }
    }
    while (UartBus.available()) {
      Serial.write(UartBus.read());
    }
    delay(1);
  }
}

static void cmdRead() {
  Serial.print(F("Number of bytes (0 = unlimited, max 4096): "));
  int maxBytes = CommandLine::readInt(0, 4096);
  if (maxBytes == -1) return;

  Serial.print(F("Timeout (ms, 0 = no timeout, max 60000): "));
  int timeoutMs = CommandLine::readInt(0, 60000);
  if (timeoutMs == -1) return;

  if (maxBytes == 0 && timeoutMs == 0) timeoutMs = 1000;

  uint8_t buf[4096];
  int total = 0;
  unsigned long start = millis();

  while (true) {
    if (maxBytes > 0 && total >= maxBytes) break;
    if (timeoutMs > 0 && millis() - start >= (unsigned long)timeoutMs) break;
    if (total >= 4096) break;
    if (UartBus.available()) {
      buf[total++] = UartBus.read();
    } else {
      delay(1);
    }
  }

  if (total == 0) {
    Serial.println(F("No data received."));
    return;
  }

  for (int i = 0; i < total; i += 16) {
    if (i < 0x100000) Serial.print("0");
    if (i < 0x10000) Serial.print("0");
    if (i < 0x1000) Serial.print("0");
    if (i < 0x100) Serial.print("0");
    if (i < 0x10) Serial.print("0");
    Serial.print(i, HEX);
    Serial.print(F(": "));

    for (int j = 0; j < 16; j++) {
      if (i + j < total) {
        if (buf[i + j] < 0x10) Serial.print("0");
        Serial.print(buf[i + j], HEX);
        Serial.print(" ");
      } else {
        Serial.print(F("   "));
      }
      if (j == 7) Serial.print(" ");
    }

    Serial.print(F("  "));
    for (int j = 0; j < 16 && i + j < total; j++) {
      uint8_t b = buf[i + j];
      Serial.print((b >= 32 && b <= 126) ? (char)b : '.');
    }
    Serial.println();
  }
}

static void cmdWrite() {
  Serial.print(F("Mode (text/hex): "));
  String mode = CommandLine::readInput();
  if (CommandLine::isAbortCommand(mode)) return;
  mode.toLowerCase();
  mode.trim();

  if (mode == "text" || mode == "t") {
    Serial.print(F("String to send: "));
    String text = CommandLine::readInput();
    if (CommandLine::isAbortCommand(text)) return;

    Serial.print(F("Append \\r\\n? (y/n): "));
    while (true) {
      if (!Serial.available()) { delay(10); continue; }
      char c = Serial.read();
      if (c == 'y' || c == 'Y') {
        Serial.println('y');
        UartBus.print(text);
        UartBus.print("\r\n");
        Serial.print(F("Sent: "));
        Serial.print(text);
        Serial.println(F(" [with \\r\\n]"));
        break;
      } else if (c == 'n' || c == 'N') {
        Serial.println('n');
        UartBus.print(text);
        Serial.print(F("Sent: "));
        Serial.println(text);
        break;
      }
    }
  } else if (mode == "hex" || mode == "h") {
    Serial.print(F("Hex bytes (space-separated, e.g. 48 65 6C): "));
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return;

    int count = 0;
    int start = 0;
    while (start < (int)line.length()) {
      while (start < (int)line.length() && line[start] == ' ') start++;
      if (start >= (int)line.length()) break;
      int end = start;
      while (end < (int)line.length() && line[end] != ' ') end++;
      String byteStr = line.substring(start, end);
      byteStr.trim();
      if (byteStr.length() > 0) {
        int val = strtol(byteStr.c_str(), nullptr, 16);
        UartBus.write((uint8_t)(val & 0xFF));
        count++;
      }
      start = end;
    }
    Serial.print(F("Sent "));
    Serial.print(count);
    Serial.println(F(" hex bytes."));
  } else {
    Serial.println(F("Invalid mode. Use text or hex."));
  }
}

static void cmdPing() {
  while (UartBus.available()) UartBus.read();
  UartBus.write(0x55);
  delay(200);
  if (UartBus.available()) {
    Serial.println(F("Device responded."));
    while (UartBus.available()) {
      uint8_t b = UartBus.read();
      if (b < 0x10) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println(F("No response."));
  }
}

void UartMode::begin() {
  Preferences prefs;
  prefs.begin("nexus_uart", false);
  if (prefs.isKey("tx")) {
    currentTx = prefs.getInt("tx", PIN_UART_TX);
    currentRx = prefs.getInt("rx", PIN_UART_RX);
    currentBaud = prefs.getInt("baud", 9600);
    currentParity = prefs.getInt("parity", 0);
    currentStopBits = prefs.getInt("stop", 1);
  }
  prefs.end();
  applyUart();
}

void UartMode::handle(const String& cmd) {
  String command = cmd;
  command.trim();

  if (command == "help") {
    printUartHelp();
  } else if (command == "config") {
    cmdConfig();
  } else if (command == "save") {
    cmdSave();
  } else if (command == "scan") {
    cmdScan();
  } else if (command == "bridge") {
    cmdBridge();
  } else if (command == "read") {
    cmdRead();
  } else if (command == "write") {
    cmdWrite();
  } else if (command == "ping") {
    cmdPing();
  } else {
    String upper = command;
    upper.toUpperCase();
    Serial.print("[UART] ");
    Serial.print(upper);
    Serial.println(" \xe2\x80\x94 not implemented yet.");
  }
}
