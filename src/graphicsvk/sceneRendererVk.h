#pragma once

#include "graphics/sceneRenderer.h"

class SceneRendererVk : public SceneRenderer
{
public:
  // rendertarget should either be some kind of generic image container or a swapchain tied to a window.
  // the render call can modify it.
  // camera and scene should not be modified.  Renderer could modify its own internal data
  virtual void Render(RenderTarget* target, const Camera& camera, const Scene& scene, float simulationTime = 0) override
  {}

  // each renderer should have its own particular options related to the render algorithm.
};