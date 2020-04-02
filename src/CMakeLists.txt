# let external dir be included everywhere
include_directories(
    external/glm
    external/KTX-Software
    external/stb
    external/tinygltf
)

add_subdirectory(agave2app)
add_subdirectory(external)
