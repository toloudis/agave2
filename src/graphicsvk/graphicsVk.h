
#pragma once

#include "graphics/graphics.h"

class GraphicsVk : public Graphics
{
public:
  GraphicsVk();
  ~GraphicsVk();

  bool init() override;
  bool cleanup() override;

  SceneRenderer* createDefaultRenderer() override;
  SceneRenderer* createNormalsRenderer() override;

  RenderTarget* createWindowRenderTarget() override;
  RenderTarget* createImageRenderTarget() override;
};
