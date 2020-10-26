#include "camera.h"

#include <stdio.h>

void Camera::processKeyboard(CameraMovement direction, float deltaTime){
  float velocity = movementSpeed * deltaTime;
  if(fast){
    velocity *= 10.0f;
  }

  if(direction == FORWARD){
    position += front * velocity;
  }
  if(direction == BACKWARD){
    position -= front * velocity;
  }
  if(direction == LEFT){
    position -= right * velocity;
  }
  if(direction == RIGHT){
    position += right * velocity;
  }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch){
  xoffset *= mouseSensitivity;
  yoffset *= mouseSensitivity;

  yaw += xoffset;
  pitch += yoffset;

  if(constrainPitch){
    if(pitch > 90.0f){
      pitch = 90.0f;
    }
    if(pitch < -90.0f){
      pitch = -90.0f;
    }
  }

  updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset){
  fov -= yoffset * 2.0f;
  if(fov < 1.0f){
    fov = 1.0f;
  }
  if(fov > FOV){
    fov = FOV;
  }
}

void Camera::updateCameraVectors(){
  glm::vec3 _front;
  _front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  _front.y = sin(glm::radians(pitch));
  _front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

  front = glm::normalize(_front);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}
