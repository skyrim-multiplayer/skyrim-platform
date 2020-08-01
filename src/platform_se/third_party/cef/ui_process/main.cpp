#include "OverlayApp.hpp"
#include "ProcessHandler.h"
#include <filesystem>
#include <fstream>
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
  std::function<TiltedPhoques::OverlayRenderProcessHandler*()> f = []() {
    return new ProcessHandler;
  };
  [&]() {
    __try {
      TiltedPhoques::UIMain(lpCmdLine, hInstance, f);
    } __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
  }();
}
