#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "win/winutil.h"
#include <string>
#include <vector>
#include <windows.h>

class Keyboard {
  private:
    std::string name;

  public:
    winutil::Handle m_device;

    Keyboard(winutil::Handle);
    Keyboard(HANDLE);

    std::string getName();
};

class KeyboardDevices {
  public:
    std::vector<Keyboard> getList() const;
};
#endif
