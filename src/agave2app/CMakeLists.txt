add_executable(agave2app MACOSX_BUNDLE "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
set_target_properties(agave2app PROPERTIES OUTPUT_NAME "agave")
target_include_directories(agave2app PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${Vulkan_INCLUDE_DIRS}
)

target_sources(agave2app PRIVATE
)

target_link_libraries(agave2app 
    ${Vulkan_LIBRARIES}
#    render
    base
    graphics
    graphicsvk
)
