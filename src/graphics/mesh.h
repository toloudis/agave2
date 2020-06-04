#pragma once

#include "boundingBox.h"

class Mesh
{
public:
  virtual ~Mesh() {}
  virtual BoundingBox getBoundingBox() = 0;
};
