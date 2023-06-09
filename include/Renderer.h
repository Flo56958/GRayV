#pragma once

#include "Shader.h"
#include "Camera.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Vulkan/Vulkan.h>
#include <vector>
#include <set>
#include <string>
#include <limits>
#include <algorithm>

#define GLSL_450( x ) "#version 450\n" #x

class Renderer {
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void render();
	void reloadModifiedShaders();

	void setCamera(Camera* cam) { camera = cam; }

private:
	const int MAX_FRAMES_IN_FLIGHT = 2;
	uint32_t currentFrame = 0;

	GLFWwindow* window;

	VkInstance instance = VK_NULL_HANDLE;
	// Vulkan physical device
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	// Vulkan logical device
	VkDevice device = VK_NULL_HANDLE;

	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	// sort of like a framebuffer object
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline screenQuadPipeline;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	VkDescriptorPool imguiPool;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	Shader* screenQuadVS;
	Shader* screenQuadFS;

	Camera* camera;

	void drawScreenQuad(uint32_t image_nr);
	void drawGUI(VkCommandBuffer commandbuffer);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void initImGui();

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	SwapChainSupportDetails getSwapChainSupportDetails(VkPhysicalDevice device) {
		SwapChainSupportDetails sc_details{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &sc_details.capabilities);
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			sc_details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, sc_details.formats.data());
		}
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			sc_details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, sc_details.presentModes.data());
		}

		return sc_details;
	}

	bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// check for required extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto& extension : availableExtensions)
			 requiredExtensions.erase(extension.extensionName);

		// check for adequate swap chain (framebuffer) support
		SwapChainSupportDetails sc_details = getSwapChainSupportDetails(device);

		return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			&& deviceFeatures.geometryShader
			&& requiredExtensions.empty()
			&& !sc_details.formats.empty() && !sc_details.presentModes.empty();
	}
};