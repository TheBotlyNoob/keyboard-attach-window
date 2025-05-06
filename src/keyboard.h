#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "win/winutil.h"
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>

class KeyboardAttributes {
  public:
    std::uint16_t vendorId;
    std::uint16_t productId;
    std::uint16_t revisionNumber;
};

class Keyboard {
  private:
    std::string name;
    KeyboardAttributes attributes;

  public:
    winutil::Handle m_device;

    Keyboard(winutil::Handle);

    const std::string getName() const;
    const KeyboardAttributes getAttributes() const;
};

class KeyboardDevices {
  public:
    static std::vector<Keyboard> getList();
};
#endif
