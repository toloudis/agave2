
# let external dir be included everywhere
include_directories(
    external/glm
    external/KTX-Software
    external/pugixml-1.10/src
    external/spdlog/include
    external/stb
    external/tinygltf
)

add_subdirectory(external)

add_subdirectory(graphics)
add_subdirectory(graphicsvk)
add_subdirectory(fileformats)

add_subdirectory(test)

# app comes last after deps build?
add_subdirectory(agave2app)
