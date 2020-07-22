#pragma once

#include <d3d11.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <wrl/client.h>

#include "SKCDXTypes.h"

class CDXD3DDevice;
class CDXShaderFactory;

class CDXPixelShaderCache
  : protected std::unordered_map<std::string,
                                 Microsoft::WRL::ComPtr<ID3D11PixelShader>>
{
public:
  explicit CDXPixelShaderCache(CDXShaderFactory* factory)
    : m_factory(factory)
  {
  }

  virtual Microsoft::WRL::ComPtr<ID3D11PixelShader> GetShader(
    CDXD3DDevice* device, const std::string& name);

protected:
  CDXShaderFactory* m_factory;
};