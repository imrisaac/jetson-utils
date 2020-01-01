
file(GLOB gstCameraSources *.cpp)
file(GLOB gstCameraIncludes *.h )

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(camera-viewer ${gstCameraSources})
target_link_libraries(camera-viewer jetson-utils 
                                    ${OpenCV_LIBS})

install(TARGETS camera-viewer DESTINATION bin)
