#pragma once

#include <WString.h>

namespace CommandLine {
  void begin();
  void loop();
  void flushSerialInput();
  String readInput();
  bool isAbortCommand(const String& s);
  int readInt(int minVal, int maxVal);

  extern String currentMode;
  extern String currentShell;
}
