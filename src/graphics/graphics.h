
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

class Graphics
{
public:
  virtual bool init() = 0;
  virtual bool cleanup() = 0;

  virtual SceneRenderer* CreateDefaultRenderer() = 0;
};
