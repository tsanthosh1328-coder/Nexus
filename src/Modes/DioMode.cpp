#include <Arduino.h>
#include "DioMode.h"
#include "../CommandLine.h"

static bool isValidEsp32Pin(int pin) {
  int validPins[] = {0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
  for (int i = 0; i < 26; i++) {
    if (validPins[i] == pin) return true;
  }
  return false;
}

static int readPin() {
  int pin = CommandLine::readInt(0, 39);
  if (pin == -1) return -1;
  if (!isValidEsp32Pin(pin)) {
    Serial.print("Invalid pin: GPIO");
    Serial.print(pin);
    Serial.println(" does not exist on ESP32 DevKit.");
    return -1;
  }
  return pin;
}

static int readInterval() {
  int val = CommandLine::readInt(1, 60000);
  if (val == -1) return -1;
  return val;
}

static int readDuty() {
  int val = CommandLine::readInt(0, 255);
  if (val == -1) return -1;
  return val;
}

static int readFreq() {
  int val = CommandLine::readInt(1, 40000);
  if (val == -1) return -1;
  return val;
}

static bool isPinRestricted(int pin) {
  if (pin >= 6 && pin <= 11) return true;
  if (pin >= 34 && pin <= 39) return true;
  if (pin == 0 || pin == 1 || pin == 3) return true;
  return false;
}

static bool confirmRestrictedPin(int pin) {
  if (!isPinRestricted(pin)) return true;

  const char* reason = "";
  if (pin >= 6 && pin <= 11) reason = "Internal flash pin";
  else if (pin == 0) reason = "Boot pin";
  else if (pin == 1) reason = "UART TX pin";
  else if (pin == 3) reason = "UART RX pin";
  else if (pin >= 34 && pin <= 39) reason = "Input-only pin";

  Serial.print("Warning: GPIO");
  Serial.print(pin);
  Serial.print(" (");
  Serial.print(reason);
  Serial.println(")");
  Serial.print("Continue anyway? (y/n): ");
  CommandLine::flushSerialInput();

  while (true) {
    while (!Serial.available()) { delay(10); }
    char c = Serial.read();
    if (c == 'y' || c == 'Y') {
      Serial.println('y');
      return true;
    } else if (c == 'n' || c == 'N' || c == 'e' || c == 'E') {
      Serial.println(c);
      return false;
    }
  }
}

static int promptState() {
  while (true) {
    Serial.print("State (HIGH/LOW): ");
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.toLowerCase();
    line.trim();
    if (line == "high" || line == "h" || line == "1") return HIGH;
    if (line == "low" || line == "l" || line == "0") return LOW;
    Serial.println(F("Enter HIGH or LOW."));
  }
}

static int promptPinMode() {
  while (true) {
    Serial.print("Mode (INPUT/OUTPUT): ");
    String line = CommandLine::readInput();
    if (CommandLine::isAbortCommand(line)) return -1;
    line.toLowerCase();
    line.trim();
    if (line == "input") return INPUT;
    if (line == "output") return OUTPUT;
    Serial.println(F("Valid: INPUT, OUTPUT"));
  }
}

static void printDioHelp() {
  Serial.println(F("╔══════════════╦═══════════════════════════════╗"));
  Serial.println(F("║           NEXUS - DIO                        ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║ read         ║ Read digital pin state        ║"));
  Serial.println(F("║ set          ║ Set digital pin HIGH/LOW      ║"));
  Serial.println(F("║ config       ║ Configure pin mode            ║"));
  Serial.println(F("║ pwm          ║ PWM output on pin             ║"));
  Serial.println(F("║ scan         ║ Scan all GPIO states          ║"));
  Serial.println(F("║ sniff        ║ Monitor pin changes           ║"));
  Serial.println(F("║ toggle       ║ Toggle pin at interval        ║"));
  Serial.println(F("║ pulse        ║ Generate a pulse              ║"));
  Serial.println(F("║ measure      ║ Measure frequency on pin      ║"));
  Serial.println(F("║ pullup       ║ Enable internal pull-up       ║"));
  Serial.println(F("║ pulldown     ║ Enable internal pull-down     ║"));
  Serial.println(F("║ reset        ║ Reset pin to INPUT            ║"));
  Serial.println(F("║ pins         ║ Show GPIO reference table     ║"));
  Serial.println(F("╚══════════════╩═══════════════════════════════╝"));
}

static void printPins() {
  Serial.println(F("╔══════════════╦═══════════════════════════════╗"));
  Serial.println(F("║           NEXUS - DIO PINS                   ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║  RESTRICTED                                  ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║ GPIO0        ║ Boot pin — use with caution   ║"));
  Serial.println(F("║ GPIO1        ║ UART TX — avoid               ║"));
  Serial.println(F("║ GPIO3        ║ UART RX — avoid               ║"));
  Serial.println(F("║ GPIO6-11     ║ Internal flash — never use    ║"));
  Serial.println(F("║ GPIO34-39    ║ Input only — no OUTPUT/PWM    ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║  INVALID (DO NOT EXIST)                      ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║ GPIO20, 24,  ║ Not present on ESP32 DevKit   ║"));
  Serial.println(F("║ 28, 29, 30,  ║                               ║"));
  Serial.println(F("║ 31           ║                               ║"));
  Serial.println(F("╠══════════════╬═══════════════════════════════╣"));
  Serial.println(F("║ All other unlisted GPIOs are safe for general║"));
  Serial.println(F("║ use.                                         ║"));
  Serial.println(F("╚══════════════╩═══════════════════════════════╝"));
}

static void cmdRead() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  int state = digitalRead(pin);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.print(": ");
  Serial.println(state == HIGH ? "HIGH" : "LOW");
}

static void cmdSet() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  int state = promptState();
  if (state == -1) return;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, state);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.print(" set to ");
  Serial.println(state == HIGH ? "HIGH" : "LOW");
}

static void cmdConfig() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  int mode = promptPinMode();
  if (mode == -1) return;
  pinMode(pin, mode);

  Serial.print("GPIO");
  Serial.print(pin);
  Serial.print(" configured as ");
  Serial.println(mode == INPUT ? "INPUT" : "OUTPUT");
}

static void cmdPwm() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  Serial.print("Frequency (Hz): ");
  int freq = readFreq();
  if (freq == -1) return;

  Serial.print("Duty cycle (0-255): ");
  int duty = readDuty();
  if (duty == -1) return;

  ledcAttach(pin, freq, 8);
  ledcWrite(pin, duty);

  Serial.print("PWM on GPIO");
  Serial.print(pin);
  Serial.print(" at ");
  Serial.print(freq);
  Serial.print(" Hz, duty ");
  Serial.println(duty);
}

static void cmdScan() {
  int safePins[] = {0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
  const int COL1 = 14;
  const int COL2 = 12;
  const int COL3 = 28;

  Serial.println(F("╔══════════════╦════════════╦════════════════════════════╗"));
  Serial.println(F("║ PIN          ║ MODE       ║ STATE                      ║"));
  Serial.println(F("╠══════════════╬════════════╬════════════════════════════╣"));

  for (int i = 0; i < (int)(sizeof(safePins) / sizeof(safePins[0])); i++) {
    int pin = safePins[i];
    int state = digitalRead(pin);
    uint32_t enable;
    if (pin < 32) {
      enable = REG_READ(GPIO_ENABLE_REG);
    } else {
      enable = REG_READ(GPIO_ENABLE1_REG);
      pin -= 32;
    }
    bool isOutput = (enable >> pin) & 1;
    pin = safePins[i];

    String pinStr = "GPIO";
    if (pin < 10) pinStr += " ";
    pinStr += String(pin);
    Serial.print(F("║ "));
    Serial.print(pinStr);
    int pad = COL1 - 1 - pinStr.length();
    for (int j = 0; j < pad; j++) Serial.print(' ');
    Serial.print(F("║ "));

    const char* modeStr = isOutput ? "OUTPUT" : "INPUT";
    Serial.print(modeStr);
    pad = COL2 - 1 - strlen(modeStr);
    for (int j = 0; j < pad; j++) Serial.print(' ');
    Serial.print(F("║ "));

    const char* stateStr = (state == HIGH) ? "HIGH" : "LOW";
    Serial.print(stateStr);
    pad = COL3 - 1 - strlen(stateStr);
    for (int j = 0; j < pad; j++) Serial.print(' ');
    Serial.println(F("║"));
  }

  Serial.println(F("╚══════════════╩════════════╩════════════════════════════╝"));
}

static void cmdSniff() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  Serial.print("Sniffing GPIO");
  Serial.print(pin);
  Serial.println(". Press any key to stop.");

  int lastState = digitalRead(pin);
  Serial.print("0 ms - GPIO");
  Serial.print(pin);
  Serial.print(": ");
  Serial.println(lastState == HIGH ? "HIGH" : "LOW");

  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }
    int state = digitalRead(pin);
    if (state != lastState) {
      Serial.print(millis());
      Serial.print(" ms - GPIO");
      Serial.print(pin);
      Serial.print(": ");
      Serial.println(state == HIGH ? "HIGH" : "LOW");
      lastState = state;
    }
    delay(1);
  }
}

static void cmdToggle() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  Serial.print("Interval (ms): ");
  int interval = readInterval();
  if (interval == -1) return;

  pinMode(pin, OUTPUT);
  bool state = LOW;

  Serial.print("Toggling GPIO");
  Serial.print(pin);
  Serial.print(" every ");
  Serial.print(interval);
  Serial.println(" ms. Press any key to stop.");

  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }
    state = !state;
    digitalWrite(pin, state);
    Serial.print(millis());
    Serial.print(" ms - GPIO");
    Serial.print(pin);
    Serial.print(": ");
    Serial.println(state == HIGH ? "HIGH" : "LOW");
    delay(interval);
  }
}

static void cmdPulse() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  Serial.print("Duration (ms): ");
  int duration = readInterval();
  if (duration == -1) return;

  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.print(" HIGH for ");
  Serial.print(duration);
  Serial.println(" ms");
  delay(duration);
  digitalWrite(pin, LOW);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.println(" LOW — pulse complete.");
}

static volatile int edgeCount = 0;

static void IRAM_ATTR isrRising() {
  edgeCount = edgeCount + 1;
}

static void cmdMeasure() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  Serial.print("Measuring frequency on GPIO");
  Serial.println(pin);
  Serial.println("Press any key to stop.");

  while (true) {
    if (Serial.available()) {
      Serial.read();
      break;
    }

    edgeCount = 0;
    attachInterrupt(digitalPinToInterrupt(pin), isrRising, RISING);

    unsigned long start = millis();
    while (millis() - start < 1000) {
      delay(10);
    }

    detachInterrupt(digitalPinToInterrupt(pin));

    Serial.print(edgeCount);
    Serial.println(" Hz");
  }
}

static void cmdPullup() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  pinMode(pin, INPUT_PULLUP);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.println(" pull-up enabled.");
}

static void cmdPulldown() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  pinMode(pin, INPUT_PULLDOWN);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.println(" pull-down enabled.");
}

static void cmdReset() {
  Serial.print("Pin number: ");
  int pin = readPin();
  if (pin == -1) return;
  if (!confirmRestrictedPin(pin)) return;

  pinMode(pin, INPUT);
  Serial.print("GPIO");
  Serial.print(pin);
  Serial.println(" reset to INPUT.");
}

void DioMode::handle(const String& cmd) {
  String command = cmd;
  command.trim();

  if (command == "help") {
    printDioHelp();
  } else if (command == "read") {
    cmdRead();
  } else if (command == "set") {
    cmdSet();
  } else if (command == "config") {
    cmdConfig();
  } else if (command == "pwm") {
    cmdPwm();
  } else if (command == "scan") {
    cmdScan();
  } else if (command == "sniff") {
    cmdSniff();
  } else if (command == "toggle") {
    cmdToggle();
  } else if (command == "pulse") {
    cmdPulse();
  } else if (command == "measure") {
    cmdMeasure();
  } else if (command == "pullup") {
    cmdPullup();
  } else if (command == "pulldown") {
    cmdPulldown();
  } else if (command == "reset") {
    cmdReset();
  } else if (command == "pins") {
    printPins();
  } else {
    String upper = command;
    upper.toUpperCase();
    Serial.print("[DIO] ");
    Serial.print(upper);
    Serial.println(" — not implemented yet.");
  }
}
