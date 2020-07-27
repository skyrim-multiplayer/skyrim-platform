#pragma once
#include <include/cef_render_handler.h>

namespace TiltedPhoques
{
    struct OverlayClient;
    struct OverlayRenderHandler : CefRenderHandler
    {
        virtual void Reset() = 0;
        virtual void Render() = 0;
        virtual void Create() = 0;

        void SetVisible(const bool aVisible) noexcept
        {
            m_visible = aVisible;
        }

        [[nodiscard]] bool IsVisible() const noexcept
        {
            return m_visible;
        }

        void SetCursorLocation(const uint16_t aX, const uint16_t aY) noexcept
        {
            m_cursorX = aX;
            m_cursorY = aY;
        }

        [[nodiscard]] std::pair<uint16_t, uint16_t> GetCursorLocation() const noexcept
        {
            return std::make_pair(m_cursorX, m_cursorY);
        }

        void SetParent(OverlayClient* apParent) noexcept
        {
            m_pParent = apParent;
        }

    protected:

        bool m_visible{ false };
        uint16_t m_cursorX{ 0 };
        uint16_t m_cursorY{ 0 };
        OverlayClient* m_pParent{ nullptr };
    };
}
