#include "graphicsVk.h"

GraphicsVk::GraphicsVk() {}

GraphicsVk::~GraphicsVk() {}

bool
GraphicsVk::init()
{
  return false;
}

bool
GraphicsVk::cleanup()
{
  return false;
}

SceneRenderer*
GraphicsVk::createDefaultRenderer()
{
  return nullptr;
}

SceneRenderer*
GraphicsVk::createNormalsRenderer()
{
  return nullptr;
}

RenderTarget*
GraphicsVk::createWindowRenderTarget()
{
  return nullptr;
}

RenderTarget*
GraphicsVk::createImageRenderTarget()
{
  return nullptr;
}
