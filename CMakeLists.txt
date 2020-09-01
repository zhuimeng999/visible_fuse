cmake_minimum_required(VERSION 3.18)
project(visible_fuse)

set(CMAKE_CXX_STANDARD 17)

include_directories(inc)
add_executable(${PROJECT_NAME}  src/main/main.cpp
        src/vulkan_utils/Vki.cpp inc/vulkan_utils/Vki.h
        src/vulkan_utils/Vkd.cpp inc/vulkan_utils/Vkd.h
        src/vulkan_utils/Vkc.cpp inc/vulkan_utils/Vkc.h
        src/vulkan_utils/Vkb.cpp inc/vulkan_utils/Vkb.h
        src/vulkan_utils/Vktexture.cpp inc/vulkan_utils/Vktexture.h
        src/vulkan_utils/Vkm.cpp inc/vulkan_utils/Vkm.h
        src/vulkan_utils/Vkp.cpp inc/vulkan_utils/Vkp.h
        src/vulkan_utils/Vkpc.cpp inc/vulkan_utils/Vkpc.h
        src/vulkan_utils/Vkpg.cpp inc/vulkan_utils/Vkpg.h
        src/utils/DepthMapFilter.cpp inc/utils/DepthMapFilter.h
        src/utils/DebugDisplay.cpp inc/utils/DebugDisplay.h
        src/utils/ply.cc inc/utils/ply.h
        inc/utils/endian.h src/utils/LRC_cpu.cpp inc/utils/LRC_cpu.h)

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES})

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})

find_package(Boost REQUIRED COMPONENTS filesystem program_options log)
target_link_libraries(${PROJECT_NAME} Boost::filesystem Boost::program_options Boost::log)

find_package(OpenImageIO REQUIRED)
target_link_libraries(${PROJECT_NAME}  OpenImageIO)
add_custom_command(
  TARGET ${PROJECT_NAME}  POST_BUILD
  COMMAND rsync -rv ${CMAKE_SOURCE_DIR}/data/glsl/ $<TARGET_FILE_DIR:${PROJECT_NAME}>/glsl/
  COMMENT "Copying shaders" VERBATIM
)

find_package( OpenCV REQUIRED )
target_include_directories( ${PROJECT_NAME} SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS} )
target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS})

  include(ExternalProject)
ExternalProject_Add(glfw3
  PREFIX 3rdparty
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.2
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD})
ExternalProject_Get_Property(glfw3 install_dir)
include_directories(${install_dir}/include)
add_dependencies(${PROJECT_NAME}  glfw3)
target_link_libraries(${PROJECT_NAME}  ${install_dir}/lib/libglfw3.a dl pthread X11)