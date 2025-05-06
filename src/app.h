#include "keyboard.h"
#include <unordered_map>
#include <vector>

class KeyboardState {
  public:
    Keyboard keyboard;
    bool listening;
}

class AppContext {
    std::unordered_map<KeyboardAttributes, Keyboard> keyboardList;
};
