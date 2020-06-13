#include "volumeDimensions.h"

#include "spdlog/spdlog.h"

#include <set>

bool
startsWith(std::string mainStr, std::string toMatch)
{
  // std::string::find returns 0 if toMatch is found at starting
  if (mainStr.find(toMatch) == 0)
    return true;
  else
    return false;
}

bool
VolumeDimensions::validate() const
{
  bool ok = true;
  if (dimensionOrder == "") {
    spdlog::error("Dimension order is null");
    ok = false;
  }
  if (!startsWith(dimensionOrder, "XY") && !startsWith(dimensionOrder, "YX")) {
    spdlog::error("Invalid dimension order: {}", dimensionOrder);
    ok = false;
  }
  // legal characters in DimensionOrder:
  static const std::set<char> standardDims = { 'X', 'Y', 'Z', 'C', 'T' };
  // check for any dups or extra
  std::set<char> dimsInDimOrder;
  bool dups = false;
  bool badDims = false;
  for (auto d : dimensionOrder) {
    if (dimsInDimOrder.count(d) == 0) {
      dimsInDimOrder.insert(d);
    } else {
      dups = true;
    }
    if (standardDims.count(d) == 0) {
      badDims = true;
    }
  }
  if (dups || badDims) {
    spdlog::error("Invalid dimension order: {}", dimensionOrder);
    ok = false;
  }

  if (sizeX <= 0) {
    spdlog::error("Invalid X size: {}", sizeX);
    ok = false;
  }
  if (sizeY <= 0) {
    spdlog::error("Invalid Y size: {}", sizeY);
    ok = false;
  }
  if (sizeZ <= 0) {
    spdlog::error("Invalid Z size: {}", sizeZ);
    ok = false;
  }
  if (sizeC <= 0) {
    spdlog::error("Invalid C size: {}", sizeC);
    ok = false;
  }
  if (sizeT <= 0) {
    spdlog::error("Invalid T size: {}", sizeT);
    ok = false;
  }

  if (physicalSizeX <= 0) {
    spdlog::error("Invalid physical pixel size x: {}", physicalSizeX);
    ok = false;
  }
  if (physicalSizeY <= 0) {
    spdlog::error("Invalid physical pixel size y: {}", physicalSizeY);
    ok = false;
  }
  if (physicalSizeZ <= 0) {
    spdlog::error("Invalid physical pixel size z: {}", physicalSizeZ);
    ok = false;
  }

  if (!channelNames.empty() && channelNames.size() != sizeC) {
    spdlog::error("Invalid number of channel names: {} for {} channels", channelNames.size(), sizeC);
    ok = false;
  }

  return ok;
}

uint32_t
VolumeDimensions::getPlaneIndex(uint32_t z, uint32_t c, uint32_t t) const
{
  size_t iz = dimensionOrder.find('Z') - 2;
  size_t ic = dimensionOrder.find('C') - 2;
  size_t it = dimensionOrder.find('T') - 2;
  // assuming dims.validate() == true :
  // assert (iz < 0 || iz > 2 || ic < 0 || ic > 2 || it < 0 || it > 2);

  // check SizeZ
  if (z < 0 || z >= sizeZ) {
    spdlog::error("Invalid Z index: {}/{}", z, sizeZ);
  }

  // check SizeC
  if (c < 0 || c >= sizeC) {
    spdlog::error("Invalid C index: {}/{}", c, sizeC);
  }

  // check SizeT
  if (t < 0 || t >= sizeT) {
    spdlog::error("Invalid T index: {}/{}", t, sizeT);
  }

  // assign rasterization order
  int v0 = iz == 0 ? z : (ic == 0 ? c : t);
  int v1 = iz == 1 ? z : (ic == 1 ? c : t);
  int v2 = iz == 2 ? z : (ic == 2 ? c : t);
  int len0 = iz == 0 ? sizeZ : (ic == 0 ? sizeC : sizeT);
  int len1 = iz == 1 ? sizeZ : (ic == 1 ? sizeC : sizeT);

  return v0 + v1 * len0 + v2 * len0 * len1;
}

std::vector<uint32_t>
VolumeDimensions::getPlaneZCT(uint32_t planeIndex) const
{
  size_t iz = dimensionOrder.find('Z') - 2;
  size_t ic = dimensionOrder.find('C') - 2;
  size_t it = dimensionOrder.find('T') - 2;
  // assuming dims.validate() == true :
  // assert (iz < 0 || iz > 2 || ic < 0 || ic > 2 || it < 0 || it > 2);

  // check image count
  if (planeIndex < 0 || planeIndex >= sizeZ * sizeC * sizeT) {
    spdlog::error("Invalid image index: {}/{}", planeIndex, (sizeZ * sizeC * sizeT));
  }

  // assign rasterization order
  int len0 = iz == 0 ? sizeZ : (ic == 0 ? sizeC : sizeT);
  int len1 = iz == 1 ? sizeZ : (ic == 1 ? sizeC : sizeT);
  // int len2 = iz == 2 ? sizeZ : (ic == 2 ? sizeC : sizeT);
  int v0 = planeIndex % len0;
  int v1 = planeIndex / len0 % len1;
  int v2 = planeIndex / len0 / len1;
  uint32_t z = iz == 0 ? v0 : (iz == 1 ? v1 : v2);
  uint32_t c = ic == 0 ? v0 : (ic == 1 ? v1 : v2);
  uint32_t t = it == 0 ? v0 : (it == 1 ? v1 : v2);

  return { z, c, t };
}

void
VolumeDimensions::log() const
{
  spdlog::info("Begin VolumeDimensions");
  spdlog::info("sizeX: {}", sizeX);
  spdlog::info("sizeY: {}", sizeY);
  spdlog::info("sizeZ: {}", sizeZ);
  spdlog::info("sizeC: {}", sizeC);
  spdlog::info("sizeT: {}", sizeT);
  spdlog::info("DimensionOrder: {}", dimensionOrder);
  spdlog::info("PhysicalPixelSize: [{},{},{}]", physicalSizeX, physicalSizeY, physicalSizeZ);
  spdlog::info("bitsPerPixel: {}", bitsPerPixel);
  spdlog::info("End VolumeDimensions");
}
