#include "BrowserApi.h"
#include "NullPointerException.h"
#include <cef/hooks/DInputHook.hpp>
#include <cef/ui/OverlayApp.hpp>
#include <cef/ui/OverlayRenderHandlerD3D11.hpp>
#include <skse64/GameMenus.h>

namespace {
thread_local bool g_cursorIsOpenByFocus = false;
}

JsValue BrowserApi::SetVisible(const JsFunctionArguments& args)
{
  bool& v = TiltedPhoques::OverlayRenderHandlerD3D11::Visible();
  v = (bool)args[1];
  return JsValue::Undefined();
}

JsValue BrowserApi::SetFocused(const JsFunctionArguments& args)
{
  bool& v = TiltedPhoques::DInputHook::ChromeFocus();
  bool newFocus = (bool)args[1];
  if (v != newFocus) {
    v = newFocus;

    auto mm = MenuManager::GetSingleton();
    if (!mm)
      return JsValue::Undefined();

    static const auto fsCursorMenu = new BSFixedString("Cursor Menu");
    const bool alreadyOpen = mm->IsMenuOpen(fsCursorMenu);

    if (newFocus) {
      if (!alreadyOpen) {
        CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)
        (fsCursorMenu, UIMessage::kMessage_Open, NULL);
        g_cursorIsOpenByFocus = true;
      }
    } else {
      if (g_cursorIsOpenByFocus) {
        CALL_MEMBER_FN(UIManager::GetSingleton(), AddMessage)
        (fsCursorMenu, UIMessage::kMessage_Close, NULL);
        g_cursorIsOpenByFocus = false;
      }
    }
  }
  return JsValue::Undefined();
}

JsValue BrowserApi::LoadUrl(const JsFunctionArguments& args,
                            std::shared_ptr<State> state)
{
  if (!state)
    throw NullPointerException("state");
  if (!state->overlayService)
    throw NullPointerException("overlayApp");
  auto app = state->overlayService->GetOverlayApp();
  if (!app)
    throw NullPointerException("app");

  auto wstr = (std::wstring)args[1];
  return JsValue::Bool(app->LoadUrl(wstr.data()));
}

JsValue BrowserApi::GetToken(const JsFunctionArguments& args)
{
  return OverlayApp::GetCurrentSpToken();
}