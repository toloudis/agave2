add_executable(agave_test MACOSX_BUNDLE "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp")
set_target_properties(agave_test PROPERTIES OUTPUT_NAME "agave_test")
target_include_directories(agave_test PUBLIC
	"${CMAKE_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
)
target_sources(agave_test PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/test_histogram.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_timeLine.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_volumeDimensions.cpp"
)

target_link_libraries(agave_test 
	graphics 
)

add_custom_command(TARGET agave_test POST_BUILD
        COMMAND agave_test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/test
        COMMENT "Running tests..."
        )
