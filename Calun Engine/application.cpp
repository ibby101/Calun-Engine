#include "src/application.h"

#include <SDL3/SDL.h>
#define VOLK_IMPLEMENTATION
#include <Volk/volk.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <iostream>

void Application::showError(const std::string& errorMessage) const
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", errorMessage.c_str(), window);
}

bool Application::initialise()
{
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Calun", width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if (!window)
	{
		showError("Error creating window");
		return false;
	}

	if (!initialiseVulkan())
	{
		return false;
	}

	return true;
}

void Application::shutdown()
{
	vkDeviceWaitIdle(device);


	if (timelineSemaphore)
	{
		vkDestroySemaphore(device, timelineSemaphore, nullptr);
	}

	for (auto& res : frameResources)
	{
		vkDestroySemaphore(device, res.imageAcquiredSemaphore, nullptr);
		vkDestroyCommandPool(device, res.commandPool, nullptr);
	}

	if (pipelineLayout)
	{
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	}

	if (pipeline)
	{
		vkDestroyPipeline(device, pipeline, nullptr);
	}

	if (vertShader)
	{
		vkDestroyShaderModule(device, vertShader, nullptr);
	}

	if (fragShader)
	{
		vkDestroyShaderModule(device, fragShader, nullptr);
	}

	destroySwapchain();

	if (vmaAllocator)
	{
		vmaDestroyAllocator(vmaAllocator);
	}

	if (surface)
	{
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
	}

	if (device)
	{
		vkDestroyDevice(device, nullptr);
	}

	if (vulkanInstance)
	{
		vkDestroyInstance(vulkanInstance, nullptr);
	}

	volkFinalize();

	if (window)
	{
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
}

void Application::run()
{
	running = true;
	while (running)
	{
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT)
			{
				running = false;
				break;
			}

			else if (event.type == SDL_EVENT_WINDOW_RESIZED)
			{
				width = event.window.data1;
				height = event.window.data2;
				break;
			}
		}

		render();
	}

}

bool Application::initialiseVulkan()
{
	if (!createVulkanInstance())
	{
		showError("Couldn't create a vulkan instance");
		return false;
	}

	if (!createSurface())
	{
		showError("Couldn't create window surface");
		return false;
	}

	if (physicalDevice = findPhysicalDevice(); !physicalDevice)
	{
		showError("Unable to find appropriate physical device");
		return false;
	}

	if (!findGraphicsQueue())
	{
		showError("Unable to find compatible graphics queue");
		return false;
	}

	if (!createDevice(physicalDevice))
	{
		showError("Couldn't create the logical GPU device");
		return false;
	}

	if (!initialiseVMA())
	{
		showError("Unable to create vulkan memory allocator");
		return false;
	}


	if (!createSwapchain(width, height))
	{
		showError("Unable to create swapchain");
		return false;
	}

	if (!createShaders())
	{
		showError("Error creating shader modules");
		return false;
	}

	if (pipeline = createGraphicsPipeline(); !pipeline)
	{
		showError("Unable to initialise the graphics pipeline");
		return false;
	}

	if (!createSyncResources())
	{
		showError("Could not create sync related resources");
		return false;
	}

	if (!createCommandBuffers())
	{
		showError("Could not create command buffer objects");
		return false;
	}

	return true;
}

bool Application::createVulkanInstance()
{
	if (volkInitialize() != VK_SUCCESS)
	{
		showError("Error initialising Volk");
		return false;
	}

	VkApplicationInfo appInfo
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "2026 Engine",
		.apiVersion = VulkanVersion,
	};

	uint32_t instExtCount = 0;
	const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&instExtCount);

	std::vector<const char*> requestedLayers
	{
		"VK_LAYER_KHRONOS_validation"
	};

	VkInstanceCreateInfo instCreateInfo
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requestedLayers.size()),
		.ppEnabledLayerNames = requestedLayers.data(),
		.enabledExtensionCount = instExtCount,
		.ppEnabledExtensionNames = extensions
	};

	if (vkCreateInstance(&instCreateInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
	{
		return false;
	}

	volkLoadInstance(vulkanInstance);
	return true;
}

bool Application::createSurface()
{
	if (!SDL_Vulkan_CreateSurface(window, vulkanInstance, nullptr, &surface))
	{
		return false;
	}

	return true;
}

VkPhysicalDevice Application::findPhysicalDevice()
{
	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDevices(physDeviceCount);
	vkEnumeratePhysicalDevices(vulkanInstance, &physDeviceCount, physicalDevices.data());

	VkPhysicalDevice physicalDevice = nullptr;
	if (physDeviceCount)
	{
		physicalDevice = physicalDevices[0];

		for (auto& physDev : physicalDevices)
		{
			VkPhysicalDeviceProperties props{};
			vkGetPhysicalDeviceProperties(physDev, &props);
			if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				physicalDevice = physDev;
				break;
			}
		}
	}

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

	bool formatSupported = false;

	for (const VkSurfaceFormatKHR& surfformat : surfaceFormats)
	{
		if (surfformat.format == swapchainFormat)
		{
			formatSupported = true;
			break;
		}
	}

	if (!formatSupported)
	{
		showError("Requested swapchain format is not supported by the surface");
		return nullptr;
	}

	return physicalDevice;
}

