#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <Vulkan/Vulkan.h>

#define GLSL_450( x ) "#version 450\n" #x

class Renderer {
public:
	Renderer(GLFWwindow* window);
	~Renderer();

	void render();

private:
	GLFWwindow* window;

	VkInstance instance;
	// Vulkan physical device
	VkPhysicalDevice physicalDevice;
	// Vulkan logical device
	VkDevice device;

	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkPipeline screenQuadPipeline;

	void drawScreenQuad();

	bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		return deviceProperties.deviceType ==
			VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
	}
};