#pragma once

#include "boundingBox.h"

class Volume
{
public:
  virtual BoundingBox getBoundingBox() = 0;
};