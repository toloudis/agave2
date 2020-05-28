#pragma once

// abstract either a buffer of image pixels or a swapchain tied to a window
class RenderTarget
{
public:
  // flush all rendering commands and swapbuffers
  virtual void swap() = 0;
  // block till rendering is done and return pixels data as a pointer to system memory
  virtual void* getPixels(/* optional rectangle ? */) = 0;
  virtual void setSize(int width, int height) = 0;
  virtual int getWidth() = 0;
  virtual int getHeight() = 0;
};