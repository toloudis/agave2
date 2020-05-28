#pragma once

class Camera;
class RenderTarget;
class Scene;

class SceneRenderer
{
public:
  // rendertarget should either be some kind of generic image container or a swapchain tied to a window.
  // the render call can modify it.
  // camera and scene should not be modified.  Renderer could modify its own internal data
  virtual void render(RenderTarget* target, const Camera& camera, const Scene& scene, float simulationTime = 0) = 0;

  // each renderer should have its own particular options related to the render algorithm.
};