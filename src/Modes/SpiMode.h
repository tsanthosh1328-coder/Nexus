#pragma once

#include <WString.h>

namespace SpiMode {
  void begin();
  void handle(const String& cmd);

  extern int currentMosi;
  extern int currentMiso;
  extern int currentSck;
  extern int currentCs;
  extern int currentClockSpeed;
}
