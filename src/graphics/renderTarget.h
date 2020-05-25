// abstract either a buffer of image pixels or a swapchain tied to a window
class RenderTarget
{
public:
  // flush all rendering commands and swapbuffers
  virtual void swap() = 0;
  // block till rendering is done and return pixels data as a pointer to system memory
  virtual void* getPixels(/* optional rectangle ? */) = 0;
};