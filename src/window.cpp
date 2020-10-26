#include "window.h"

#include <stdio.h>
#include <stdlib.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>

Window::Window(int width, int height, const char* title){
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if(window == NULL){
    fprintf(stderr, "Failed to create window\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
}

Window::~Window(){
  glfwDestroyWindow(window);
  glfwTerminate();
}

bool Window::shouldClose() const{
  return glfwWindowShouldClose(window) != 0;
}

void Window::pollEvents() const{
  glfwPollEvents();
}

void Window::swapBuffers() const{
  glfwSwapBuffers(window);
}
