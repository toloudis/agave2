#pragma once

#include "graphics/renderTarget.h"

// abstract either a buffer of image pixels or a swapchain tied to a window
class RenderTargetVk : public RenderTarget
{
public:
  // flush all rendering commands and swapbuffers
  void swap() override {}
  // block till rendering is done and return pixels data as a pointer to system memory
  void* getPixels(/* optional rectangle ? */) override {}
};