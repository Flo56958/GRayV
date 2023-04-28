cmake_minimum_required(VERSION 3.22)

project(GRayV)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_subdirectory(external/glm)
add_subdirectory(external/glfw)
add_subdirectory(external/shaderc)
message(WARNING "Remember to run external/shaderc/utils/git-sync-deps with Python!")

# ImGui
# Set where the ImGui files are stored
set(IMGUI_PATH  external/imgui)
add_library(ImGui 
					"${IMGUI_PATH}/imgui.cpp"
					"${IMGUI_PATH}/imgui_demo.cpp"
					"${IMGUI_PATH}/imgui_draw.cpp"
					"${IMGUI_PATH}/imgui_widgets.cpp"
					"${IMGUI_PATH}/imgui_tables.cpp"
					"${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp"
					"${IMGUI_PATH}/backends/imgui_impl_glfw.cpp")
target_include_directories(ImGui
								PUBLIC "${IMGUI_PATH}"
								PUBLIC "${IMGUI_PATH}/backends"
								PUBLIC "${Vulkan_INCLUDE_DIR}")
target_link_libraries(ImGui glfw ${Vulkan_LIBRARIES})

# Glob sources of GRayV
file(GLOB_RECURSE GRAYV_SOURCES "./src/*.cpp" "./src/*.hpp")

add_executable(GRayV ${GRAYV_SOURCES})

target_include_directories(GRayV
								PUBLIC "./include/"
								PUBLIC "${GLFW_INCLUDE_DIRS}"
								PUBLIC "${Vulkan_INCLUDE_DIR}"
								PUBLIC "${libshaderc_SOURCE_DIR}/include/"
								PUBLIC "${IMGUI_PATH}")

target_link_libraries(GRayV glm::glm glfw ${Vulkan_LIBRARIES} shaderc ImGui)

# Compiler specific stuff
IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")