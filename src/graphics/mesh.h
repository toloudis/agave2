#pragma once

#include "boundingBox.h"

class Mesh
{
public:
  virtual BoundingBox getBoundingBox() = 0;
};
