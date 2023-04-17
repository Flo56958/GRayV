#include <algorithm>
#include <iostream>

#include "Renderer.h"

int main()
{
    // initialize GLFW
    glfwInit();

    // do not create an OpenGL context as we are using Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    Renderer ren(window);

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    // terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}