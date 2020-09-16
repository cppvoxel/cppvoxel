#include "input.h"

#include <map>

std::map<int, Input::KeyEvent> keyMap;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
  switch(action){
    case GLFW_PRESS:
      keyMap[key].pressed = true;
      keyMap[key].released = false;
      keyMap[key].down = true;
      break;
    case GLFW_REPEAT:
      keyMap[key].pressed = false;
      keyMap[key].released = false;
      keyMap[key].down = true;
      break;
    case GLFW_RELEASE:
      keyMap[key].pressed = false;
      keyMap[key].released = true;
      keyMap[key].down = false;
      break;
    default:
      break;
  }
}

void Input::init(GLFWwindow* window){
  glfwSetKeyCallback(window, keyCallback);
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
