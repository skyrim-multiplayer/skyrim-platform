#include "TPOverlayService.h"

#include <cef/ui/OverlayApp.hpp>

#include <D3D11Hook.hpp>

#include <cef/ui/OverlayRenderHandler.hpp>
#include <cef/ui/OverlayRenderHandlerD3D11.hpp>

#include "TPRenderSystemD3D11.h"

using TiltedPhoques::OverlayRenderHandlerD3D11;
using TiltedPhoques::OverlayRenderHandler;

struct D3D11RenderProvider final
  : OverlayApp::RenderProvider
  , OverlayRenderHandlerD3D11::Renderer
{
  explicit D3D11RenderProvider(RenderSystemD3D11* apRenderSystem)
    : m_pRenderSystem(apRenderSystem)
  {
  }

  OverlayRenderHandler* Create() override
  {
    return new OverlayRenderHandlerD3D11(this);
  }

  [[nodiscard]] HWND GetWindow() override
  {
    return m_pRenderSystem->GetWindow();
  }

  [[nodiscard]] IDXGISwapChain* GetSwapChain() const noexcept override
  {
    return m_pRenderSystem->GetSwapChain();
  }

private:
  RenderSystemD3D11* m_pRenderSystem;
};

OverlayService::OverlayService()
{
}

OverlayService::~OverlayService() noexcept
{
}

void OverlayService::Create(RenderSystemD3D11* apRenderSystem)
{
  m_pOverlay =
    new OverlayApp(std::make_unique<D3D11RenderProvider>(apRenderSystem));
  m_pOverlay->Initialize();
  m_pOverlay->GetClient()->Create();
}

void OverlayService::Render() const
{
  m_pOverlay->GetClient()->Render();
}

void OverlayService::Reset() const
{
  m_pOverlay->GetClient()->Reset();
}