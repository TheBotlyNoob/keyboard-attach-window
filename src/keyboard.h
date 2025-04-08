#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "win/winutil.h"
#include <string>
#include <vector>
#include <windows.h>

class Keyboard {
  private:
  public:
    winutil::Handle mDevHandle;

    Keyboard(winutil::Handle);

    std::string GetName();
    std::string GetId();
};

class KeyboardDevices {
  public:
    std::vector<Keyboard> GetList();
};
#endif
