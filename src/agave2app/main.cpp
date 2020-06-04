#if defined(_WIN32)
#pragma comment(linker, "/subsystem:console")
#endif

#include <algorithm>
#include <array>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/camera.h"
#include "graphics/mesh.h"
#include "graphics/renderTarget.h"
#include "graphics/scene.h"
#include "graphics/sceneObject.h"
#include "graphics/sceneRenderer.h"

#include "graphicsvk/graphicsVk.h"

#define DEBUG (!NDEBUG)

#define LOG(...) printf(__VA_ARGS__)

void
save(const char* imagedata, size_t rowPitch, int width, int height, bool colorSwizzle)
{
  if (!imagedata) {
    return;
  }
  const char* filename = "headless.ppm";

  std::ofstream file(filename, std::ios::out | std::ios::binary);

  // ppm header
  file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

  // ppm binary pixel data
  for (int32_t y = 0; y < height; y++) {
    unsigned int* row = (unsigned int*)imagedata;
    for (int32_t x = 0; x < width; x++) {
      if (colorSwizzle) {
        file.write((char*)row + 2, 1);
        file.write((char*)row + 1, 1);
        file.write((char*)row, 1);
      } else {
        file.write((char*)row, 3);
      }
      row++;
    }
    imagedata += rowPitch;
  }
  file.close();
  LOG("Framebuffer image saved to %s\n", filename);
}

int
main()
{
  Graphics* graphics = new GraphicsVk();
  bool ok = graphics->init();
  if (!ok) {
    std::cout << "Could not init Graphics";
  }
  SceneRenderer* renderer = graphics->createDefaultRenderer();
  int width = 512;
  int height = 512;
  RenderTarget* rendertarget = graphics->createImageRenderTarget(512, 512);
  Camera camera;

  Scene scene;

  float positions[] = { 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f };
  uint32_t indices[] = { 0, 1, 2 };
  Mesh* mesh = graphics->createMesh(3, positions, nullptr, nullptr, 3, indices);
  SceneObject* triangle = new SceneObject(mesh);
  scene.add(triangle);

  renderer->render(rendertarget, camera, scene);
  rendertarget->swap();
  const void* pixels = rendertarget->getPixels();
  // save pixels to file
  save(reinterpret_cast<const char*>(pixels), width * 4, width, height, false);

  std::cout << "Beginning cleanup" << std::endl;

  // clean up scene geometry
  delete mesh;

  // clean up window
  delete rendertarget;

  // clean up renderer
  delete renderer;

  // clean up graphics system
  delete graphics;

  std::cout << "Done!" << std::endl;
  return 0;
}
