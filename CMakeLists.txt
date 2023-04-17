cmake_minimum_required(VERSION 3.22)

project(GRayV)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glm)
add_subdirectory(external/glfw)

file(GLOB_RECURSE sources "./src/*.cpp" "./src/*.hpp")

add_executable(GRayV ${sources})

target_include_directories(GRayV
								PUBLIC "${CMAKE_BINARY_DIR}/include/"
								PUBLIC "${GLFW_INCLUDE_DIRS}"
								PUBLIC "${Vulkan_INCLUDE_DIR}")

target_link_libraries(GRayV glm::glm glfw ${Vulkan_LIBRARIES})

# Compiler specific stuff
IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")