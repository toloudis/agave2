#include "fileReaderCzi.h"

#include "graphics/boundingBox.h"
#include "graphics/imageXYZC.h"
#include "graphics/volumeDimensions.h"

#include "pugixml.hpp"
#include "spdlog/spdlog.h"

#include <libCZI/Src/libCZI/libCZI.h>

#include <chrono>
#include <codecvt>
#include <locale>
#include <map>
#include <set>

static const int IN_MEMORY_BPP = 16;

FileReaderCzi::FileReaderCzi() {}

FileReaderCzi::~FileReaderCzi() {}

class ScopedCziReader
{
public:
  ScopedCziReader(const std::string& filepath)
  {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring widestr = converter.from_bytes(filepath);

    std::shared_ptr<libCZI::IStream> stream = libCZI::CreateStreamFromFile(widestr.c_str());
    m_reader = libCZI::CreateCZIReader();

    m_reader->Open(stream);
  }
  ~ScopedCziReader()
  {
    if (m_reader) {
      m_reader->Close();
    }
  }
  std::shared_ptr<libCZI::ICZIReader> reader() { return m_reader; }

protected:
  std::shared_ptr<libCZI::ICZIReader> m_reader;
};

libCZI::IntRect
getSceneYXSize(libCZI::SubBlockStatistics& statistics, int sceneIndex = 0)
{
  bool hasScene = statistics.dimBounds.IsValid(libCZI::DimensionIndex::S);
  if (hasScene) {
    int sStart(0), sSize(0);
    statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::S, &sStart, &sSize);
    if (sceneIndex >= sStart && (sStart + sSize - 1) >= sceneIndex && !statistics.sceneBoundingBoxes.empty()) {
      return statistics.sceneBoundingBoxes[sceneIndex].boundingBoxLayer0;
    }
  } else {
    return statistics.boundingBoxLayer0Only;
  }
  return statistics.boundingBoxLayer0Only;
}

bool
readCziDimensions(const std::shared_ptr<libCZI::ICZIReader>& reader,
                  const std::string filepath,
                  libCZI::SubBlockStatistics& statistics,
                  VolumeDimensions& dims)
{
  // check for mosaic.  we can't (won't) handle those right now.
  if (statistics.maxMindex > 0) {
    spdlog::error("CZI file is mosaic; mosaic reading not yet implemented");
    return false;
  }

  // metadata xml
  auto mds = reader->ReadMetadataSegment();
  std::shared_ptr<libCZI::ICziMetadata> md = mds->CreateMetaFromMetadataSegment();
  std::shared_ptr<libCZI::ICziMultiDimensionDocumentInfo> docinfo = md->GetDocumentInfo();

  libCZI::ScalingInfo scalingInfo = docinfo->GetScalingInfo();
  // convert meters to microns?
  dims.physicalSizeX = scalingInfo.scaleX * 1000000.0f;
  dims.physicalSizeY = scalingInfo.scaleY * 1000000.0f;
  dims.physicalSizeZ = scalingInfo.scaleZ * 1000000.0f;

  // get all dimension bounds and enumerate
  statistics.dimBounds.EnumValidDimensions([&](libCZI::DimensionIndex dimensionIndex, int start, int size) -> bool {
    switch (dimensionIndex) {
      case libCZI::DimensionIndex::Z:
        dims.sizeZ = size;
        break;
      case libCZI::DimensionIndex::C:
        dims.sizeC = size;
        break;
      case libCZI::DimensionIndex::T:
        dims.sizeT = size;
        break;
      default:
        break;
    }
    return true;
  });

  libCZI::IntRect planebox = getSceneYXSize(statistics);
  dims.sizeX = planebox.w;
  dims.sizeY = planebox.h;

  std::string xml = md->GetXml();
  // parse the xml
  pugi::xml_document czixml;
  pugi::xml_parse_result result = czixml.load_string(xml.c_str());
  if (!result) {
    spdlog::error("XML parse error:");
    spdlog::error("Error description: {}", result.description());
    spdlog::error("Error offset: {} {error at [...{}]", result.offset, (xml.c_str() + result.offset));
    spdlog::error("Bad CZI xml metadata content");
    return false;
  }

  pugi::xml_node metadataEl = czixml.child("Metadata");
  if (!metadataEl) {
    spdlog::error("No Metadata element in czi xml");
    return false;
  }
  pugi::xml_node informationEl = metadataEl.child("Information");
  if (!informationEl) {
    return false;
  }
  pugi::xml_node imageEl = informationEl.child("Image");
  if (!imageEl) {
    return false;
  }
  pugi::xml_node dimensionsEl = imageEl.child("Dimensions");
  if (!dimensionsEl) {
    return false;
  }
  pugi::xml_node channelsEl = dimensionsEl.child("Channels");
  if (!channelsEl) {
    return false;
  }
  std::vector<std::string> channelNames;
  for (pugi::xml_node channelEl = channelsEl.child("Channel"); channelEl;
       channelEl = channelEl.next_sibling("Channel")) {
    channelNames.push_back(channelEl.attribute("Name").value());
  }

  dims.channelNames = channelNames;

  libCZI::SubBlockInfo info;
  bool ok = reader->TryGetSubBlockInfoOfArbitrarySubBlockInChannel(0, info);
  if (ok) {
    switch (info.pixelType) {
      case libCZI::PixelType::Gray8:
        dims.bitsPerPixel = 8;
        break;
      case libCZI::PixelType::Gray16:
        dims.bitsPerPixel = 16;
        break;
      case libCZI::PixelType::Gray32Float:
        dims.bitsPerPixel = 32;
        break;
      case libCZI::PixelType::Bgr24:
        dims.bitsPerPixel = 24;
        break;
      case libCZI::PixelType::Bgr48:
        dims.bitsPerPixel = 48;
        break;
      case libCZI::PixelType::Bgr96Float:
        dims.bitsPerPixel = 96;
        break;
      default:
        dims.bitsPerPixel = 0;
        return false;
    }
  } else {
    return false;
  }

  return dims.validate();
}

// DANGER: assumes dataPtr has enough space allocated!!!!
bool
readCziPlane(const std::shared_ptr<libCZI::ICZIReader>& reader,
             const libCZI::IntRect& planeRect,
             const libCZI::CDimCoordinate& planeCoord,
             const VolumeDimensions& volumeDims,
             uint8_t* dataPtr)
{
  reader->EnumSubset(&planeCoord, &planeRect, true, [&](int idx, const libCZI::SubBlockInfo& info) -> bool {
    // accept first subblock
    std::shared_ptr<libCZI::ISubBlock> subblock = reader->ReadSubBlock(idx);

    std::shared_ptr<libCZI::IBitmapData> bitmap = subblock->CreateBitmap();
    // and copy memory
    libCZI::IntSize size = bitmap->GetSize();
    {
      libCZI::ScopedBitmapLockerSP lckScoped{ bitmap };
      assert(lckScoped.ptrDataRoi == lckScoped.ptrData);
      assert(volumeDims.sizeX == size.w);
      assert(volumeDims.sizeY == size.h);
      size_t bytesPerRow = size.w * 2; // destination stride
      if (volumeDims.bitsPerPixel == 16) {
        assert(lckScoped.stride >= size.w * 2);
        // stridewise copying
        for (std::uint32_t y = 0; y < size.h; ++y) {
          const std::uint8_t* ptrLine = ((const std::uint8_t*)lckScoped.ptrDataRoi) + y * lckScoped.stride;
          // uint16 is 2 bytes per pixel
          memcpy(dataPtr + (bytesPerRow * y), ptrLine, bytesPerRow);
        }
      } else if (volumeDims.bitsPerPixel == 8) {
        assert(lckScoped.stride >= size.w);
        // stridewise copying
        for (std::uint32_t y = 0; y < size.h; ++y) {
          const std::uint8_t* ptrLine = ((const std::uint8_t*)lckScoped.ptrDataRoi) + y * lckScoped.stride;
          uint16_t* destLine = reinterpret_cast<uint16_t*>(dataPtr + (bytesPerRow * y));
          for (size_t x = 0; x < size.w; ++x) {
            *destLine++ = *(ptrLine + x);
          }
        }
      }
    }

    // stop iterating, on the assumption that there is only one subblock that fits this planecoordinate
    return false;
  });

  return true;
}

VolumeDimensions
FileReaderCzi::loadDimensionsCzi(const std::string& filepath, int32_t scene)
{
  VolumeDimensions dims;
  try {
    ScopedCziReader scopedReader(filepath);
    std::shared_ptr<libCZI::ICZIReader> cziReader = scopedReader.reader();

    auto statistics = cziReader->GetStatistics();

    bool dims_ok = readCziDimensions(cziReader, filepath, statistics, dims);
    if (!dims_ok) {
      return VolumeDimensions();
    }

    return dims;

  } catch (std::exception& e) {
    spdlog::error(e.what());
    spdlog::error("Failed to read {}", filepath);
    return VolumeDimensions();
  } catch (...) {
    spdlog::error("Failed to read {}", filepath);
    return VolumeDimensions();
  }
}

std::shared_ptr<ImageXYZC>
FileReaderCzi::loadCzi(const std::string& filepath, VolumeDimensions* outDims, int32_t time, int32_t scene)
{
  std::shared_ptr<ImageXYZC> emptyimage;

  auto tStart = std::chrono::high_resolution_clock::now();

  try {
    ScopedCziReader scopedReader(filepath);
    std::shared_ptr<libCZI::ICZIReader> cziReader = scopedReader.reader();

    auto statistics = cziReader->GetStatistics();

    VolumeDimensions dims;
    bool dims_ok = readCziDimensions(cziReader, filepath, statistics, dims);
    if (!dims_ok) {
      return emptyimage;
    }
    int startT = 0, sizeT = 0;
    int startC = 0, sizeC = 0;
    int startZ = 0, sizeZ = 0;
    int startS = 0, sizeS = 0;
    bool hasT = statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::T, &startT, &sizeT);
    bool hasZ = statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::Z, &startZ, &sizeZ);
    bool hasC = statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &startC, &sizeC);
    bool hasS = statistics.dimBounds.TryGetInterval(libCZI::DimensionIndex::S, &startS, &sizeS);

    if (!hasZ) {
      spdlog::error("Agave can only read zstack volume data");
      return emptyimage;
    }
    if (dims.sizeC != sizeC || !hasC) {
      spdlog::error("Inconsistent Channel count in czi file");
      return emptyimage;
    }

    size_t planesize = dims.sizeX * dims.sizeY * dims.bitsPerPixel / 8;
    uint8_t* data = new uint8_t[planesize * dims.sizeZ * dims.sizeC];
    memset(data, 0, planesize * dims.sizeZ * dims.sizeC);

    // stash it here in case of early exit, it will be deleted
    std::unique_ptr<uint8_t[]> smartPtr(data);

    uint8_t* destptr = data;

    // now ready to read channels one by one.

    for (uint32_t channel = 0; channel < dims.sizeC; ++channel) {
      for (uint32_t slice = 0; slice < dims.sizeZ; ++slice) {
        destptr = data + planesize * (channel * dims.sizeZ + slice);

        // adjust coordinates by offsets from dims
        libCZI::CDimCoordinate planeCoord{ { libCZI::DimensionIndex::Z, (int)slice + startZ } };
        if (hasC) {
          planeCoord.Set(libCZI::DimensionIndex::C, (int)channel + startC);
        }
        if (hasS) {
          planeCoord.Set(libCZI::DimensionIndex::S, scene + startS);
        }
        if (hasT) {
          planeCoord.Set(libCZI::DimensionIndex::T, time + startT);
        }

        if (!readCziPlane(cziReader, statistics.boundingBoxLayer0Only, planeCoord, dims, destptr)) {
          return emptyimage;
        }
      }
    }

    auto tEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = tEnd - tStart;
    spdlog::debug("CZI loaded in {} ms", elapsed.count() * 1000.0);

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

  } catch (std::exception& e) {
    spdlog::error(e.what());
    spdlog::error("Failed to read {}", filepath);
    return emptyimage;
  } catch (...) {
    spdlog::error("Failed to read {}", filepath);
    return emptyimage;
  }
}
