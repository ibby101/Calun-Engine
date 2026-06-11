cmake_minimum_required(VERSION 3.20)
project(Calun-Engine)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)
find_package(glm REQUIRED)

add_executable(calunapp
	"Calun Engine/src/main.cpp"
	"Calun Engine/src/application.h"
	"Calun Engine/src/application.cpp"
	"Calun Engine/src/utils.h"
	"Calun Engine/src/utils.cpp"
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