#pragma once

#define VULKAN_NO_PROTOTYPES
#include <SDL3/SDL_Vulkan.h>
#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <shaderc/shaderc.hpp>

struct SDL_Window;
struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;

struct FrameResources
{
	VkCommandPool commandPool = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	VkSemaphore imageAcquiredSemaphore = nullptr;
};

class Application
{
	constexpr static uint32_t VulkanVersion{ VK_API_VERSION_1_4 };
	constexpr static uint32_t MaxFramesInFlight{ 2 };
	constexpr static uint32_t
};
