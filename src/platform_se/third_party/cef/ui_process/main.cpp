#include "OverlayApp.hpp"
#include "ProcessHandler.h"
#include <windows.h>

#include <fstream>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
  //std::ofstream("debugf.txt") << "1";
  TiltedPhoques::UIMain(lpCmdLine, hInstance,
                        []() { return new ProcessHandler; });
}
