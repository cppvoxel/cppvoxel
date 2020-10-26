#ifndef WINDOW_H_
#define WINDOW_H_

struct GLFWwindow;

class Window{
public:
  GLFWwindow* window;

  Window(int width, int height, const char* title);
  ~Window();

  bool shouldClose() const;
  void pollEvents() const;
  void swapBuffers() const;
};

#endif
