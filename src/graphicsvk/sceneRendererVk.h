#pragma once

#include "graphics/sceneRenderer.h"

namespace vks {
struct VulkanDevice;
}

class SceneRendererVk : public SceneRenderer
{
public:
  SceneRendererVk(vks::VulkanDevice* vulkanDevice);

  // rendertarget should either be some kind of generic image container or a swapchain tied to a window.
  // the render call can modify it.
  // camera and scene should not be modified.  Renderer could modify its own internal data
  virtual void render(RenderTarget* target,
                      const Camera& camera,
                      const Scene& scene,
                      float simulationTime = 0) override;

  // each renderer should have its own particular options related to the render algorithm.

private:
  vks::VulkanDevice* m_device = nullptr;
};