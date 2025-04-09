#ifndef GUI_H
#define GUI_H

#include "imgui.h"
#include "keyboard.h"

class AppGui {
  private:
    bool show_demo_window = true;
    bool show_another_window = false;
    ImFont *font;
    KeyboardDevices deviceController;
    std::vector<Keyboard> deviceList;
    std::vector<std::string> deviceNames;
    std::string name = "HEY!";

  public:
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO &io;

    KeyboardDevices keyboards;

    AppGui();

    void InitImGui();

    void ImGuiNewFrame();
};

#endif
