#pragma once

#include <WString.h>

namespace I2cMode {
  void begin();
  void handle(const String& cmd);
}
