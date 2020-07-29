#include "OverlayApp.hpp"
#include "ProcessHandler.h"
#include <filesystem>
#include <fstream>
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
  TiltedPhoques::UIMain(lpCmdLine, hInstance,
                        []() { return new ProcessHandler; });
}
