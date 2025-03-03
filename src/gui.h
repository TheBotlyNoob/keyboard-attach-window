#ifndef GUI_H
#define GUI_H

#include "imgui.h"

class AppGui {
  private:
    bool show_demo_window = true;
    bool show_another_window = false;
    ImFont *font;

  public:
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO &io;

    AppGui();

    void InitImGui();

    void ImGuiNewFrame();
};

#endif
