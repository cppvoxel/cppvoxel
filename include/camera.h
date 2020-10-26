#ifndef CAMERA_H_
#define CAMERA_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMovement{
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 20.0f;
const float SENSITIVITY = 0.05f;
const float FOV = 70.0f;

class Camera{
public:
  glm::vec3 position;
  glm::vec3 front;
  float fov;
  bool fast;
  float yaw;
  float pitch;

  Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f), float _yaw = YAW, float _pitch = PITCH) : front(glm::vec3(0.0f, 0.0f, -1.0f)), fov(FOV), fast(false), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY){
    position = _position;
    worldUp = _up;
    yaw = _yaw;
    pitch = pitch;
    updateCameraVectors();
  }
  Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float _yaw, float _pitch) : front(glm::vec3(0.0f, 0.0f, -1.0f)), fov(FOV), fast(false), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY){
    position = glm::vec3(posX, posY, posZ);
    worldUp = glm::vec3(upX, upY, upZ);
    yaw = _yaw;
    pitch = pitch;
    updateCameraVectors();
  }

  glm::mat4 getViewMatrix(){
    return glm::lookAt(position, position + front, up);
  }

  void processKeyboard(CameraMovement direction, float deltaTime);
  void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
  void processMouseScroll(float yoffset);

private:
  glm::vec3 up;
  glm::vec3 right;
  glm::vec3 worldUp;

  float movementSpeed;
  float mouseSensitivity;

  void updateCameraVectors();
};

#endif
