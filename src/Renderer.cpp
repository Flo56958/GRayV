#include "Renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Vulkan/Vulkan.hpp>

static const auto screenQuad_VS = GLSL_450(
	layout(location = 0) out vec2 outUV;

	void main()
	{
		outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
		gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);
	}
);

Renderer::Renderer(GLFWwindow* window) : window(window)
{
	// init Vulkan
	// optional application Info, more information for the driver
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "GRayV Renderer";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// not optional
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// get required Vulkan extensions from GLFW
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	// no validation layers
	createInfo.enabledLayerCount = 0;

	// create Vulkan instance
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vulkan instance!");
	}

	// create Window Surface for Vulkan
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Window Surface in GLFW for Vulkan!");
	}

	// check and pick Vulkan device
	{
		// check available Vulkan devices
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");
		}

		// get available Vulkan devices
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to find a suitable GPU for Vulkan rendering!");
		}
	}

	// create logical device
	uint32_t graphicsFamily = -1, presentFamily = -1;
	{
		// get graphics family of the physical device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsFamily = i;
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
				if (presentSupport) {
					presentFamily = i;
				}
			}
			i++;
		}
		if (graphicsFamily == -1 || presentFamily == -1) {
			throw std::runtime_error("Failed to retrieve Graphics or Present Family!");
		}

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { graphicsFamily, presentFamily };
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // device specific extensions
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		createInfo.enabledLayerCount = 0;

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create logical device!");
		}

		vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
	}

	// configuring swap chain (framebuffer)
	{
		SwapChainSupportDetails sc_details = getSwapChainSupportDetails(physicalDevice);
		VkSurfaceFormatKHR swapSurfaceFormat;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // this one is on every device
		VkExtent2D swapExtend;

		// choose surface format
		{
			bool found = false;
			for (const auto& availableFormat : sc_details.formats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
					availableFormat.colorSpace ==
					VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					swapSurfaceFormat = availableFormat;
					found = true;
					break;
				}
			}

			if (!found) {
				swapSurfaceFormat = sc_details.formats[0];
			}
		}

		// choose presentation mode
		for (const auto& availablePresentMode : sc_details.presentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				presentMode = availablePresentMode;
			}
		}

		// choose swap extend (resolution)
		if (sc_details.capabilities.currentExtent.width !=
			std::numeric_limits<uint32_t>::max()) {
			swapExtend = sc_details.capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			actualExtent.width = std::clamp(actualExtent.width,
				sc_details.capabilities.minImageExtent.width,
				sc_details.capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height,
				sc_details.capabilities.minImageExtent.height,
				sc_details.capabilities.maxImageExtent.height);
			swapExtend = actualExtent;
		}

		// images in swap chain (min + 1 to not wait on driver)
		uint32_t imageCount = sc_details.capabilities.minImageCount + 1;
		if (sc_details.capabilities.maxImageCount > 0 && imageCount >
			sc_details.capabilities.maxImageCount) {
			imageCount = sc_details.capabilities.maxImageCount;
		}

		// creating the swap chain
		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = swapSurfaceFormat.format;
		createInfo.imageColorSpace = swapSurfaceFormat.colorSpace;
		createInfo.imageExtent = swapExtend;
		createInfo.imageArrayLayers = 1; // always 1 as we are not rendering in 3D
		createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		if (graphicsFamily != presentFamily) {
			uint32_t queueFamily[2] = { graphicsFamily, presentFamily };
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamily;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = sc_details.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Swap Chain!");
		}

		// retrieve swap chain images
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageFormat = swapSurfaceFormat.format;
		swapChainExtent = swapExtend;
	}

	// create image views
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create Image Views!");
			}

		}
	}


	// continue at page 74

	// For the Screen Quad Render
	// Create 3 Verticies with no information (info is added later in vertex shader)
	//VkGraphicsPipelineCreateInfo pipelineCreateInfo = VkGraphicsPipelineCreateInfo(graphics.pipelineLayout,
	//																				renderPass,
	//																				0);

	//VkPipelineVertexInputStateCreateInfo emptyInputState;
	//emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	//emptyInputState.vertexAttributeDescriptionCount = 0;
	//emptyInputState.pVertexAttributeDescriptions = nullptr;
	//emptyInputState.vertexBindingDescriptionCount = 0;
	//emptyInputState.pVertexBindingDescriptions = nullptr;
	//pipelineCreateInfo.pVertexInputState = &emptyInputState;
	//vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &screenQuadPipeline);
}

Renderer::~Renderer()
{
	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
};

void Renderer::render()
{
	drawScreenQuad();
}

void Renderer::drawScreenQuad()
{
	//vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuadPipeline);
	//vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}