
# let external dir be included everywhere
include_directories(
    external/glm
    external/KTX-Software
    external/stb
    external/tinygltf
)

add_subdirectory(external)

add_subdirectory(graphics)
add_subdirectory(graphicsvk)

# app comes last after deps build?
add_subdirectory(agave2app)
