#include "glfw/glfw.h"

#include <stdio.h>

namespace GLFW {

double getTime() {
  return glfwGetTime();
}

void enableVsync(bool value) {
  glfwSwapInterval(value ? 1 : 0);
}

}
