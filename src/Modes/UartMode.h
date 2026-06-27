#pragma once

#include <WString.h>

namespace UartMode {
  void begin();
  void handle(const String& cmd);
}
