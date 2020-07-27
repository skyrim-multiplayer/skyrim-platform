#include <OverlayApp.hpp>
#include <filesystem>
#include <Filesystem.hpp>

namespace TiltedPhoques
{
    OverlayApp::OverlayApp(std::unique_ptr<RenderProvider> apRenderProvider, std::wstring aProcessName) noexcept
        : m_pBrowserProcessHandler(new OverlayBrowserProcessHandler)
        , m_pRenderProvider(std::move(apRenderProvider))
        , m_processName(std::move(aProcessName))
    {
    }

    void OverlayApp::Initialize() noexcept
    {
        if (m_pGameClient)
            return;

        CefMainArgs args(GetModuleHandleA(nullptr));

        const auto currentPath = TiltedPhoques::GetPath();

        CefSettings settings;

        settings.no_sandbox = true;
        settings.multi_threaded_message_loop = true;
        settings.windowless_rendering_enabled = true;

#ifdef DEBUG
        settings.log_severity = LOGSEVERITY_VERBOSE;
        settings.remote_debugging_port = 8384;
#else
        settings.log_severity = LOGSEVERITY_DEFAULT;
#endif

        CefString(&settings.log_file).FromWString(currentPath / L"logs" / L"cef_debug.log");
        CefString(&settings.cache_path).FromWString(currentPath / L"cache");

        CefString(&settings.browser_subprocess_path).FromWString(currentPath / m_processName);

        CefInitialize(args, settings, this, nullptr);

        m_pGameClient = new OverlayClient(m_pRenderProvider->Create());

        CefBrowserSettings browserSettings{};

        browserSettings.file_access_from_file_urls = STATE_ENABLED;
        browserSettings.universal_access_from_file_urls = STATE_ENABLED;
        browserSettings.web_security = STATE_DISABLED;
        browserSettings.windowless_frame_rate = 60;

        CefWindowInfo info;
        info.SetAsWindowless(m_pRenderProvider->GetWindow());

        CefBrowserHost::CreateBrowser(info, m_pGameClient.get(), (currentPath / L"Data" / L"Online" / L"UI" / L"index.html").wstring(), browserSettings, nullptr, nullptr);
    }

    void OverlayApp::ExecuteAsync(const std::string& acFunction, const CefRefPtr<CefListValue>& apArguments) const noexcept
    {
        if (!m_pGameClient)
            return;

        auto pMessage = CefProcessMessage::Create("browser-event");
        auto pArguments = pMessage->GetArgumentList();

        const auto pFunctionArguments = apArguments ? apArguments : CefListValue::Create();

        pArguments->SetString(0, acFunction);
        pArguments->SetList(1, pFunctionArguments);

        auto pBrowser = m_pGameClient->GetBrowser();
        if (pBrowser)
        {
            pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, pMessage);
        }
    }

    void OverlayApp::InjectKey(const cef_key_event_type_t aType, const uint32_t aModifiers, const uint16_t aKey, const uint16_t aScanCode) const noexcept
    {
        if (m_pGameClient && m_pGameClient->IsReady())
        {
            CefKeyEvent ev;

            ev.type = aType;
            ev.modifiers = aModifiers;
            ev.windows_key_code = aKey;
            ev.native_key_code = aScanCode;

            m_pGameClient->GetBrowser()->GetHost()->SendKeyEvent(ev);
        }
    }

    void OverlayApp::InjectMouseButton(const uint16_t aX, const uint16_t aY, const cef_mouse_button_type_t aButton, const bool aUp, const uint32_t aModifier) const noexcept
    {
        if (m_pGameClient && m_pGameClient->IsReady())
        {
            CefMouseEvent ev;

            ev.x = aX;
            ev.y = aY;
            ev.modifiers = aModifier;

            m_pGameClient->GetBrowser()->GetHost()->SendMouseClickEvent(ev, aButton, aUp, 1);
        }
    }

    void OverlayApp::InjectMouseMove(const uint16_t aX, const uint16_t aY, const uint32_t aModifier) const noexcept
    {
        if (m_pGameClient && m_pGameClient->IsReady())
        {
            CefMouseEvent ev;

            ev.x = aX;
            ev.y = aY;
            ev.modifiers = aModifier;

            m_pGameClient->GetOverlayRenderHandler()->SetCursorLocation(aX, aY);

            m_pGameClient->GetBrowser()->GetHost()->SendMouseMoveEvent(ev, false);
        }
    }

    void OverlayApp::InjectMouseWheel(const uint16_t aX, const uint16_t aY, const int16_t aDelta, const uint32_t aModifier) const noexcept
    {
        if (m_pGameClient && m_pGameClient->IsReady())
        {
            CefMouseEvent ev;

            ev.x = aX;
            ev.y = aY;
            ev.modifiers = aModifier;

            m_pGameClient->GetBrowser()->GetHost()->SendMouseWheelEvent(ev, 0, aDelta);
        }
    }

    void OverlayApp::OnBeforeCommandLineProcessing(const CefString& aProcessType, CefRefPtr<CefCommandLine> aCommandLine)
    {
    }
}
