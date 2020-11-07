#ifndef WINDOW_H_
#define WINDOW_H_

#include <GL/glew.h>
#include <glfw/glfw3.h>

namespace GLFW{

typedef void mouse_callback_t(double, double);

enum CursorMode{
  NORMAL = GLFW_CURSOR_NORMAL,
  HIDDEN = GLFW_CURSOR_HIDDEN,
  DISABLED = GLFW_CURSOR_DISABLED
};

class Window{
public:
  Window(int width, int height, const char* title);
  ~Window();

  bool shouldClose() const;
  void pollEvents() const;
  void swapBuffers() const;

  void getSize(int *width, int *height);
  bool getFullscreen();

  void setShouldClose(bool value);
  void setCursorMode(CursorMode mode);
  void setFullscreen(bool fullscreen);

  void setMouseCallback(mouse_callback_t* callback);

private:
  GLFWwindow* window;
  int windowedXPos, windowedYPos, windowedWidth, windowedHeight;

  mouse_callback_t* externalMouseCallback;

  void mouseCallback(double x, double y);
  static void cursorPosCallback(GLFWwindow* window, double x, double y);
};

}

#endif
