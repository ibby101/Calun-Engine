cmake_minimum_required(VERSION 3.20)
project(Calun-Engine)

find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)
find_package(glm REQUIRED)

add_executable(calunapp
	"src/main.cpp"
	"src/application.h"
	"src/application.cpp"
	"src/utils.h"
	"src/utils.cpp"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	target_compile_options(calunapp PRIVATE /std:c++latest)
endif()

target_link_libraries(calunapp PRIVATE
	Vulkan::Vulkan
	Vulkan::volk
	Vulkan::shaderc_combined
	SDL3::SDL3
	glm::glm)

if(UNIX AND NOT APPLE)
    target_link_libraries(calunapp PRIVATE
        SPIRV-Tools-opt
        SPIRV-Tools
        glslang
        MachineIndependent)
endif()

if(WIN32)
add_custom_command(TARGET calunapp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:SDL3::SDL3>
	$<TARGET_FILE_DIR:calunapp>
)
endif()

add_custom_command(TARGET calunapp POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
    "$<TARGET_FILE_DIR:calunapp>/src/shaders"
    COMMENT "Syncing updated GLSL shaders to execution folder..."
)
