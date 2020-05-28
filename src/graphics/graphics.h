
#pragma once

#include <cstdint>

class Mesh;
class RenderTarget;
class SceneRenderer;

// Graphics* graphics = new GraphicsVk();
// bool ok = graphics->init();
// if (!ok) then fail
// SceneRenderer* r = graphics->CreateDefaultRenderer();
// // create window.
// RenderTarget* tgt = graphics->createWindowRenderTarget();
// or
// RenderTarget* tgt = graphics->createImageRenderTarget();
// r->Render(tgt);
// tgt->Swap() // flush
// tgt->GetImage() // get pixels
// graphics->cleanup()

enum PixelFormat
{
  RGBA8U,
  RGBA32F
};

class Graphics
{
public:
  Graphics() {}
  virtual ~Graphics() {}

  virtual bool init() = 0;
  virtual bool cleanup() = 0;

  virtual SceneRenderer* createDefaultRenderer() = 0;
  virtual SceneRenderer* createNormalsRenderer() = 0;
  // virtual ScenePickRenderer* r = graphics->CreatePickRenderer(); // separate interface?

  virtual RenderTarget* createWindowRenderTarget() = 0;
  virtual RenderTarget* createImageRenderTarget(int width, int height, PixelFormat format = PixelFormat::RGBA8U) = 0;

  virtual Mesh* createMesh(uint32_t i_nVertices,
                           const float* i_Vertices,
                           const float* i_Normals,
                           const float* i_UVs,
                           uint32_t i_nIndices,
                           const uint32_t* i_Indices) = 0;
};
