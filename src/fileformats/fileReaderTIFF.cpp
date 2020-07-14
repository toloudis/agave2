#include "fileReaderTIFF.h"

#include "graphics/boundingBox.h"
#include "graphics/imageXYZC.h"
#include "graphics/volumeDimensions.h"

#include "pugixml.hpp"
#include "spdlog/spdlog.h"

#include <tiff.h>
#include <tiffio.h>

#include <chrono>
#include <map>
#include <set>

bool
startsWith(std::string const& mainStr, std::string const& toMatch)
{
  // std::string::find returns 0 if toMatch is found at starting
  if (mainStr.find(toMatch) == 0)
    return true;
  else
    return false;
}
bool
endsWith(std::string const& mainStr, std::string const& toMatch)
{
  if (mainStr.length() >= toMatch.length()) {
    return (0 == mainStr.compare(mainStr.length() - toMatch.length(), toMatch.length(), toMatch));
  } else {
    return false;
  }
}

// container supports push_back
template<class Container>
void
split(const std::string& str, Container& cont, char delim = ' ')
{
  std::stringstream ss(str);
  std::string token;
  while (std::getline(ss, token, delim)) {
    cont.push_back(token);
  }
}

static const uint32_t IN_MEMORY_BPP = 16;

FileReaderTIFF::FileReaderTIFF() {}

FileReaderTIFF::~FileReaderTIFF() {}

class ScopedTiffReader
{
public:
  ScopedTiffReader(const std::string& filepath)
  {
    // Loads tiff file
    m_tiff = TIFFOpen(filepath.c_str(), "r");
    if (!m_tiff) {
      spdlog::error("Failed to open TIFF: '{}'", filepath);
    }
  }
  ~ScopedTiffReader()
  {
    if (m_tiff) {
      TIFFClose(m_tiff);
    }
  }
  TIFF* reader() { return m_tiff; }

protected:
  TIFF* m_tiff;
};

bool
readTiffDimensions(TIFF* tiff, const std::string filepath, VolumeDimensions& dims)
{
  char* imagedescriptionptr = nullptr;
  // metadata is in ImageDescription of first IFD in the file.
  if (TIFFGetField(tiff, TIFFTAG_IMAGEDESCRIPTION, &imagedescriptionptr) != 1) {
    spdlog::error("Failed to read imagedescription of TIFF: '{}'", filepath);
    return false;
  }
  std::string imagedescription(imagedescriptionptr);

  // Temporary variables
  uint32 width, height;
  //  tsize_t scanlength;

  // Read dimensions of image
  if (TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) != 1) {
    spdlog::error("Failed to read width of TIFF: '{}'", filepath);
    return false;
  }
  if (TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) != 1) {
    spdlog::error("Failed to read height of TIFF: '{}'", filepath);
    return false;
  }

  uint32_t bpp = 0;
  if (TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bpp) != 1) {
    spdlog::error("Failed to read bpp of TIFF: '{}'", filepath);
    return false;
  }

  uint16_t sampleFormat = SAMPLEFORMAT_UINT;
  if (TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat) != 1) {
    // just warn here.  We are not yet using sampleFormat!
    spdlog::warn("Failed to read sampleformat of TIFF: '{}'", filepath);
  }

  uint32_t sizeT = 1;
  uint32_t sizeX = width;
  uint32_t sizeY = height;
  uint32_t sizeZ = 1;
  uint32_t sizeC = 1;
  float physicalSizeX = 1.0f;
  float physicalSizeY = 1.0f;
  float physicalSizeZ = 1.0f;
  std::vector<std::string> channelNames;
  std::string dimensionOrder = "XYCZT";

  // check for plain tiff with ImageJ imagedescription:
  if (startsWith(imagedescription, "ImageJ=")) {
    // "ImageJ=\nhyperstack=true\nimages=7900\nchannels=1\nslices=50\nframes=158"
    // "ImageJ=1.52i\nimages=126\nchannels=2\nslices=63\nhyperstack=true\nmode=composite\nunit=
    //      micron\nfinterval=299.2315368652344\nspacing=0.2245383462882669\nloop=false\nmin=9768.0\nmax=
    //        14591.0\n"
    std::vector<std::string> sl;
    split(imagedescription, sl, '\n');
    // split each string into name/value pairs,
    // then look up as a map.
    std::map<std::string, std::string> imagejmetadata;
    for (int i = 0; i < sl.size(); ++i) {
      std::vector<std::string> namevalue;
      split(sl[i], namevalue, '=');
      if (namevalue.size() == 2) {
        imagejmetadata[namevalue[0]] = namevalue[1];
      } else if (namevalue.size() == 1) {
        imagejmetadata[namevalue[0]] = "";
      } else {
        spdlog::error("Unexpected name/value pair in TIFF ImageJ metadata: {}", sl[i]);
        return false;
      }
    }

    if (imagejmetadata.find("channels") != imagejmetadata.end()) {
      std::string value = imagejmetadata["channels"];
      sizeC = std::stoi(value);
    } else {
      spdlog::warn("Failed to read number of channels of ImageJ TIFF: '{}'", filepath);
    }

    if (imagejmetadata.find("slices") != imagejmetadata.end()) {
      std::string value = imagejmetadata["slices"];
      sizeZ = std::stoi(value);
    } else {
      spdlog::warn("Failed to read number of slices of ImageJ TIFF: '{}'", filepath);
    }

    if (imagejmetadata.find("frames") != imagejmetadata.end()) {
      std::string value = imagejmetadata["frames"];
      sizeT = std::stoi(value);
    } else {
      spdlog::warn("Failed to read number of frames of ImageJ TIFF: '{}'", filepath);
    }

    if (imagejmetadata.find("spacing") != imagejmetadata.end()) {
      std::string value = imagejmetadata["spacing"];
      try {
        physicalSizeZ = std::stof(value);
        if (physicalSizeZ < 0.0f) {
          physicalSizeZ = -physicalSizeZ;
        }
        physicalSizeX = physicalSizeZ;
        physicalSizeY = physicalSizeZ;
      } catch (...) {
        spdlog::warn("Failed to read spacing of ImageJ TIFF: '{}'", filepath);
        physicalSizeZ = 1.0f;
      }
    }

    for (uint32_t i = 0; i < sizeC; ++i) {
      channelNames.push_back(std::to_string(i));
    }
  } else if (startsWith(imagedescription, "{\"shape\":")) {
    // expect a 4d shape array of C,Z,Y,X or 5d T,C,Z,Y,X
    int firstBracket = imagedescription.find('[');
    int lastBracket = imagedescription.rfind(']');
    std::string shape = imagedescription.substr(firstBracket + 1, lastBracket - firstBracket - 1);
    spdlog::info(shape);
    std::vector<std::string> shapelist;
    split(shape, shapelist, ',');
    if (shapelist.size() != 4 || shapelist.size() != 5) {
      spdlog::error("Expected shape to be 4D or 5D TIFF: '{}'", filepath);
      return false;
    }
    dimensionOrder = "XYZCT";
    bool hasT = (shapelist.size() == 5);
    int shapeIndex = 0;
    if (hasT) {
      sizeT = std::stoi(shapelist[shapeIndex++]);
    }
    sizeC = std::stoi(shapelist[shapeIndex++]);
    sizeZ = std::stoi(shapelist[shapeIndex++]);
    sizeY = std::stoi(shapelist[shapeIndex++]);
    sizeX = std::stoi(shapelist[shapeIndex++]);
    for (uint32_t i = 0; i < sizeC; ++i) {
      channelNames.push_back(std::to_string(i));
    }

  } else if (startsWith(imagedescription, "<?xml version") && endsWith(imagedescription, "OME>")) {
    // convert c to xml doc.  if this fails then we don't have an ome tif.
    pugi::xml_document omexml;
    pugi::xml_parse_result result = omexml.load_string(imagedescription.c_str());
    if (!result) {
      spdlog::error("XML parse error:");
      spdlog::error("Error description: {}", result.description());
      spdlog::error("Error offset: {} {error at [...{}]", result.offset, (imagedescription.c_str() + result.offset));
      spdlog::error("Bad ome xml content");
      return false;
    }

    // extract some necessary info from the xml:
    // use the FIRST Pixels element found.
    pugi::xml_node pixelsEl = omexml.child("Pixels");
    if (pixelsEl) {
      spdlog::error("No <Pixels> element in ome xml");
      return false;
    }

    // skipping "complex", "double-complex", and "bit".
    std::map<std::string, uint32_t> mapPixelTypeBPP = { { "uint8", 8 },  { "uint16", 16 }, { "uint32", 32 },
                                                        { "int8", 8 },   { "int16", 16 },  { "int32", 32 },
                                                        { "float", 32 }, { "double", 64 } };

    std::string pixelType = pixelsEl.attribute("Type").as_string("uint16");
    std::transform(
      pixelType.begin(), pixelType.end(), pixelType.begin(), [](unsigned char c) { return std::tolower(c); });
    spdlog::info("pixel type: {}", pixelType);
    bpp = mapPixelTypeBPP[pixelType];
    if (bpp != 16 && bpp != 8) {
      spdlog::error("Image must be 8 or 16-bit integer typed");
      return false;
    }
    sizeX = pixelsEl.attribute("SizeX").as_int(0);
    sizeY = pixelsEl.attribute("SizeY").as_int(0);
    sizeZ = pixelsEl.attribute("SizeZ").as_int(0);
    sizeC = pixelsEl.attribute("SizeC").as_int(0);
    sizeT = pixelsEl.attribute("SizeT").as_int(0);
    // one of : "XYZCT", "XYZTC","XYCTZ","XYCZT","XYTCZ","XYTZC"
    dimensionOrder = pixelsEl.attribute("DimensionOrder").as_string(dimensionOrder.c_str());
    physicalSizeX = pixelsEl.attribute("PhysicalSizeX").as_float(1.0f);
    physicalSizeY = pixelsEl.attribute("PhysicalSizeY").as_float(1.0f);
    physicalSizeZ = pixelsEl.attribute("PhysicalSizeZ").as_float(1.0f);
    std::string physicalSizeXunit = pixelsEl.attribute("PhysicalSizeXUnit").as_string("");
    std::string physicalSizeYunit = pixelsEl.attribute("PhysicalSizeYUnit").as_string("");
    std::string physicalSizeZunit = pixelsEl.attribute("PhysicalSizeZUnit").as_string("");

    // find channel names
    int i = 0;
    for (pugi::xml_node chel : pixelsEl.children("Channel")) {
      std::string chid = chel.attribute("ID").as_string();
      std::string chname = chel.attribute("Name").as_string();
      if (!chname.empty()) {
        channelNames.push_back(chname);
      } else if (!chid.empty()) {
        channelNames.push_back(chid);
      } else {
        channelNames.push_back(std::to_string(i));
      }
      ++i;
    }
  } else {
    // unrecognized string / no metadata.
    // walk the file and count the directories and assume that is Z
    // sizeZ was initialized to 1.
    sizeZ = 0;
    while (TIFFSetDirectory(tiff, sizeZ)) {
      sizeZ++;
    };
    channelNames.push_back("0");
  }

  assert(sizeX == width);
  assert(sizeY == height);

  // allocate the destination buffer!!!!
  assert(sizeT >= 1);
  assert(sizeC >= 1);
  assert(sizeX >= 1);
  assert(sizeY >= 1);
  assert(sizeZ >= 1);

  dims.sizeX = sizeX;
  dims.sizeY = sizeY;
  dims.sizeZ = sizeZ;
  dims.sizeC = sizeC;
  dims.sizeT = sizeT;
  dims.dimensionOrder = dimensionOrder;
  dims.physicalSizeX = physicalSizeX;
  dims.physicalSizeY = physicalSizeY;
  dims.physicalSizeZ = physicalSizeZ;
  dims.bitsPerPixel = bpp;
  dims.channelNames = channelNames;

  dims.log();

  return dims.validate();
}

// DANGER: assumes dataPtr has enough space allocated!!!!
bool
readTiffPlane(TIFF* tiff, int planeIndex, const VolumeDimensions& dims, uint8_t* dataPtr)
{
  int setdirok = TIFFSetDirectory(tiff, planeIndex);
  if (setdirok == 0) {
    spdlog::error("Bad tiff directory specified: {}", planeIndex);
    return false;
  }

  int numBytesRead = 0;
  // TODO future optimize:
  // This function is usually called in a loop. We could factor out the TIFFmalloc and TIFFfree calls.
  // Should profile to see if the repeated malloc/frees are any kind of loading bottleneck.
  if (TIFFIsTiled(tiff)) {
    tsize_t tilesize = TIFFTileSize(tiff);
    uint32 ntiles = TIFFNumberOfTiles(tiff);
    if (ntiles != 1) {
      spdlog::error("Reader doesn't support more than 1 tile per plane");
      return false;
    }
    // assuming ntiles == 1 for all IFDs
    tdata_t buf = _TIFFmalloc(tilesize);

    uint32_t i = planeIndex;

    numBytesRead = TIFFReadEncodedTile(tiff, 0, buf, tilesize);
    if (numBytesRead < 0) {
      spdlog::error("Error reading tiff tile");
      _TIFFfree(buf);
      return false;
    }
    // copy buf into data.
    if (IN_MEMORY_BPP == dims.bitsPerPixel) {
      memcpy(dataPtr, buf, numBytesRead);
    } else {
      // convert pixels
      if (dims.bitsPerPixel != 8) {
        spdlog::error("Unexpected tiff pixel size {} bits", dims.bitsPerPixel);
        _TIFFfree(buf);
        return false;
      }
      // this assumes tight packing of pixels in both buf(source) and dataptr(dest)
      uint16_t* dataptr16 = reinterpret_cast<uint16_t*>(dataPtr);
      uint8_t* tilebytes = reinterpret_cast<uint8_t*>(buf);
      for (size_t b = 0; b < numBytesRead; ++b) {
        *dataptr16 = (uint16_t)tilebytes[b];
        dataptr16++;
      }
    }

    _TIFFfree(buf);
  } else {
    // stripped.

    uint32_t i = planeIndex;

    uint32_t planeindexintiff = i;

    // Number of bytes in a decoded scanline
    tsize_t striplength = TIFFStripSize(tiff);
    tdata_t buf = _TIFFmalloc(striplength);

    uint32 nstrips = TIFFNumberOfStrips(tiff);
    // spdlog::debug(nstrips;     // num y rows
    // spdlog::debug(striplength; // x width * rows per strip
    for (tstrip_t strip = 0; strip < nstrips; strip++) {
      numBytesRead = TIFFReadEncodedStrip(tiff, strip, buf, striplength);
      if (numBytesRead < 0) {
        spdlog::error("Error reading tiff strip");
        _TIFFfree(buf);
        return false;
      }

      // copy buf into data.
      if (IN_MEMORY_BPP == dims.bitsPerPixel) {
        memcpy(dataPtr, buf, numBytesRead);
        // advance to where the next strip will go
        dataPtr += numBytesRead;
      } else {
        if (dims.bitsPerPixel != 8) {
          spdlog::error("Unexpected tiff pixel size {} bits", dims.bitsPerPixel);
          _TIFFfree(buf);
          return false;
        }
        // convert pixels
        // this assumes tight packing of pixels in both buf(source) and dataptr(dest)
        uint8_t* stripbytes = reinterpret_cast<uint8_t*>(buf);
        uint16_t* data16 = reinterpret_cast<uint16_t*>(dataPtr);
        for (size_t b = 0; b < numBytesRead; ++b) {
          *data16++ = *stripbytes++;
          dataPtr++;
          dataPtr++;
        }
      }
    }
    _TIFFfree(buf);
  }
  return true;
}

VolumeDimensions
FileReaderTIFF::loadDimensionsTiff(const std::string& filepath, int32_t scene)
{
  ScopedTiffReader tiffreader(filepath);
  TIFF* tiff = tiffreader.reader();
  // Loads tiff file
  if (!tiff) {
    return VolumeDimensions();
  }

  VolumeDimensions dims;
  bool dims_ok = readTiffDimensions(tiff, filepath, dims);
  if (!dims_ok) {
    return VolumeDimensions();
  }

  return dims;
}

std::shared_ptr<ImageXYZC>
FileReaderTIFF::loadOMETiff(const std::string& filepath, VolumeDimensions* outDims, int32_t time, int32_t scene)
{
  std::shared_ptr<ImageXYZC> emptyimage;

  auto tStart = std::chrono::high_resolution_clock::now();

  // Loads tiff file
  ScopedTiffReader tiffreader(filepath);
  TIFF* tiff = tiffreader.reader();
  if (!tiff) {
    return emptyimage;
  }

  VolumeDimensions dims;
  bool dims_ok = readTiffDimensions(tiff, filepath, dims);
  if (!dims_ok) {
    return emptyimage;
  }

  if (scene > 0) {
    spdlog::warn("Multiscene tiff not supported yet. Using scene 0");
    scene = 0;
  }
  if (time > (int32_t)(dims.sizeT - 1)) {
    spdlog::error("Time {} exceeds time samples in file: {}", time, dims.sizeT);
    return emptyimage;
  }

  spdlog::debug("Reading {} tiff...", (TIFFIsTiled(tiff) ? "tiled" : "stripped"));

  uint32_t rowsPerStrip = 0;
  if (TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip)) {
    spdlog::debug("ROWSPERSTRIP: {}", rowsPerStrip);
    uint32_t StripsPerImage = ((dims.sizeY + rowsPerStrip - 1) / rowsPerStrip);
    spdlog::debug("Strips per image: {}", StripsPerImage);
  }
  uint32_t samplesPerPixel = 0;
  if (TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel)) {
    spdlog::debug("SamplesPerPixel: {}", samplesPerPixel);
  }
  uint32_t planarConfig = 0;
  if (TIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planarConfig)) {
    spdlog::debug("PlanarConfig: {}", (planarConfig == 1 ? "PLANARCONFIG_CONTIG" : "PLANARCONFIG_SEPARATE"));
  }

  size_t planesize_bytes = dims.sizeX * dims.sizeY * (IN_MEMORY_BPP / 8);
  size_t channelsize_bytes = planesize_bytes * dims.sizeZ;
  uint8_t* data = new uint8_t[channelsize_bytes * dims.sizeC];
  memset(data, 0, channelsize_bytes * dims.sizeC);
  // stash it here in case of early exit, it will be deleted
  std::unique_ptr<uint8_t[]> smartPtr(data);

  uint8_t* destptr = data;

  // now ready to read channels one by one.
  for (uint32_t channel = 0; channel < dims.sizeC; ++channel) {
    for (uint32_t slice = 0; slice < dims.sizeZ; ++slice) {
      uint32_t planeIndex = dims.getPlaneIndex(slice, channel, time);
      destptr = data + channel * channelsize_bytes + slice * planesize_bytes;
      if (!readTiffPlane(tiff, planeIndex, dims, destptr)) {
        return emptyimage;
      }
    }
  }

  auto tEnd = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = tEnd - tStart;
  spdlog::debug("TIFF loaded in {} ms", elapsed.count() * 1000.0);

  auto tStartImage = std::chrono::high_resolution_clock::now();

  // TODO: convert data to uint16_t pixels if not already.
  // we can release the smartPtr because ImageXYZC will now own the raw data memory
  ImageXYZC* im = new ImageXYZC(dims.sizeX,
                                dims.sizeY,
                                dims.sizeZ,
                                dims.sizeC,
                                IN_MEMORY_BPP, // dims.bitsPerPixel,
                                smartPtr.release(),
                                dims.physicalSizeX,
                                dims.physicalSizeY,
                                dims.physicalSizeZ);
  im->setChannelNames(dims.channelNames);

  tEnd = std::chrono::high_resolution_clock::now();
  elapsed = tEnd - tStartImage;
  spdlog::debug("ImageXYZC prepared in {} ms", elapsed.count() * 1000.0);

  elapsed = tEnd - tStart;
  spdlog::debug("Loaded {} in {} ms", filepath, elapsed.count() * 1000.0);

  std::shared_ptr<ImageXYZC> sharedImage(im);
  if (outDims != nullptr) {
    *outDims = dims;
  }
  return sharedImage;
}
