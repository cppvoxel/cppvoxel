#include "glfw/input.h"

#include <map>

std::map<Input::Key, Input::KeyEvent> keyMap;
GLFWwindow* glfwWindow;

void keyCallback(GLFWwindow* _window, int key, int scancode, int action, int mods){
  Input::Key _key = (Input::Key)key;

  switch(action){
    case GLFW_PRESS:
      keyMap[_key].pressed = true;
      keyMap[_key].released = false;
      keyMap[_key].down = true;
      break;
    case GLFW_REPEAT:
      keyMap[_key].pressed = false;
      keyMap[_key].released = false;
      keyMap[_key].down = true;
      break;
    case GLFW_RELEASE:
      keyMap[_key].pressed = false;
      keyMap[_key].released = true;
      keyMap[_key].down = false;
      break;
    default:
      break;
  }
}

void Input::init(GLFWwindow* _window){
  glfwWindow = _window;

  glfwSetKeyCallback(glfwWindow, keyCallback);
}

void Input::update(){
  for(auto keyEvent : keyMap){
    if(keyEvent.second.pressed){
      keyEvent.second.pressed = false;
    }

    if(keyEvent.second.released){
      keyEvent.second.released = false;
    }

    keyMap[keyEvent.first] = keyEvent.second;
  }
}

Input::KeyEvent Input::getKey(Input::Key keycode){
  auto it = keyMap.find(keycode);

  if(it != keyMap.end()){
    return it->second;
  }

  return {};
}

bool Input::getMosue(Input::MouseButton button){
  return glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
}
