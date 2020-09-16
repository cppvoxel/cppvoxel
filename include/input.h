#ifndef INPUT_H_
#define INPUT_H_

#include <glfw/glfw3.h>

namespace Input{

struct KeyEvent{
  bool pressed;
  bool released;
  bool down;
};

enum Key{
  SPACE = 32,
  COMMA = 44,
  MINUS,
  PERIOD,
  SLASH,
  ZERO,
  ONE,
  TWO,
  THREE,
  FOUR,
  FIVE,
  SIX,
  SEVEN,
  EIGHT,
  NINE,
  SEMICOLON = 59,
  EQUAL = 61,
  A = 65,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  ESCAPE = 256,
  ENTER,
  TAB,
  BACKSPACE,
  INSERT,
  RIGHT = 262,
  LEFT,
  DOWN,
  UP,
  PAGE_UP,
  PAGE_DOWN,
  HOME,
  END,
  F1 = 290,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  F25,
  LEFT_SHIFT = 340,
  LEFT_CONTROL,
  LEFT_ALT,  
  LEFT_SUPER,
  RIGHT_SHIFT,
  RIGHT_CONTROL,
  RIGHT_ALT,
  RIGHT_SUPER,
  MENU
};

void init(GLFWwindow* window);
void update();
KeyEvent getKey(Key keycode);

};

#endif
