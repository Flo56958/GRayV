#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>

#include "Renderer.h"

int main()
{
	// initialize GLFW
	glfwInit();

	// do not create an OpenGL context as we are using Vulkan
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);

	Camera cam;

	Renderer ren(window);
	ren.setCamera(&cam);

	double time = glfwGetTime() * 1000;
	const float default_camera_movement_speed = 0.0005;

	while(!glfwWindowShouldClose(window)) {
		double ctime = glfwGetTime() * 1000;
		double dt_ms = ctime - time;
		time = ctime;

		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
			cam.move_forward(float(dt_ms * default_camera_movement_speed));
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			cam.move_backward(float(dt_ms * default_camera_movement_speed));
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			cam.move_left(float(dt_ms * default_camera_movement_speed));
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
			cam.move_right(float(dt_ms * default_camera_movement_speed));
		}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			cam.move_up(float(dt_ms * default_camera_movement_speed));
		}
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			cam.move_down(float(dt_ms * default_camera_movement_speed));
		}
		cam.update();
		std::cout << cam.pos.x << " " << cam.pos.y << " " << cam.pos.z << "\n";

		ren.render();
	}

	// terminate GLFW
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}