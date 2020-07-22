#pragma once

#include "SKCDXRenderState.h"
#include "SKCDXShaderFactory.h"
#include "SKCDXTextureRenderer.h"

#include "skse64/GameTypes.h"
#include "skse64/NiTypes.h"

#include "SKStringTable.h"

#include <map>
#include <vector>

class NiTexture;
class CDXPixelShaderCache;

class CDXNifTextureRenderer
  : public CDXTextureRenderer
  , public CDXRenderState
{
public:
  bool Init(CDXD3DDevice* device, CDXPixelShaderCache* cache);
  virtual ~CDXNifTextureRenderer() {}

  struct MaskData
  {
    SKEEFixedString texture;
    SKEEFixedString technique = "normal";
    UInt32 color = 0xFFFFFF;
    UInt8 textureType = 1;
  };

  bool ApplyMasksToTexture(CDXD3DDevice* device, NiPointer<NiTexture> texture,
                           std::map<SInt32, MaskData>& masks,
                           const BSFixedString& name,
                           NiPointer<NiTexture>& output);

private:
  std::vector<NiPointer<NiTexture>> m_textures;
};