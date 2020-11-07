#include "glfw/window.h"

#include <stdio.h>
#include <stdlib.h>

#include "glfw/input.h"

#include "gl/utils.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height){
  GL::viewport(width, height);
}

namespace GLFW{

Window::Window(int width, int height, const char* title){
  glfwInit();
#ifdef DEBUG
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
#endif
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

  window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if(window == NULL){
    fprintf(stderr, "Failed to create window\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  Input::init(window);

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);
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

void Window::getSize(int *width, int *height){
  glfwGetWindowSize(window, width, height);
}

void Window::setShouldClose(bool value){
  glfwSetWindowShouldClose(window, value ? 1 : 0);
}

void Window::setCursorMode(CursorMode mode){
  glfwSetInputMode(window, GLFW_CURSOR, mode);
}

bool Window::getFullscreen(){
  return glfwGetWindowMonitor(window) != NULL;
}

void Window::setFullscreen(bool fullscreen){
  if(fullscreen){
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    if(monitor){
      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      glfwGetWindowPos(window, &windowedXPos, &windowedYPos);
      glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
      glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }else{
      fprintf(stderr, "unable to get monitor\n");
      exit(-1);
    }
  }else{
    glfwSetWindowMonitor(window, NULL, windowedXPos, windowedYPos, windowedWidth, windowedHeight, 0);
  }
}

void Window::setMouseCallback(mouse_callback_t* callback){
  externalMouseCallback = callback;
}

void Window::mouseCallback(double x, double y){
  externalMouseCallback(x, y);
}

void Window::cursorPosCallback(GLFWwindow* window, double x, double y){
  static_cast<GLFW::Window*>(glfwGetWindowUserPointer(window))->mouseCallback(x, y);
}

}
