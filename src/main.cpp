#include <stdio.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Window.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#include "Chunk.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

const static char* voxelShaderVertexSource = R"(#version 330 core
layout (location = 0) in vec4 coord;
layout (location = 1) in int brightness;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

out vec3 vPosition;
out float vBrightness;
out vec2 vTexCoord;
out vec3 vNormal;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 camera_position;
uniform vec3 light_direction = normalize(vec3(-0.9, 0.9, -0.1));

float calcLight(vec3 position, vec3 lightDir, vec3 normal){
  float diffuse = max(dot(normal, lightDir), 0.0);
  /* specular */
  vec3 viewDir = normalize(camera_position - position);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);

  return diffuse + spec;
}

void main(){
  vPosition = vec3(model * vec4(coord.xyz, 1.0));
  vBrightness = float(brightness) * 0.1 + 1.0;
  vNormal = mat3(transpose(inverse(model))) * normal;
  vTexCoord = texCoord;

  // vDiffuse = calcLight(vPosition, normalize(light_direction), normal);
  vDiffuse = max(0.0, dot(normal, light_direction));

  gl_Position = projection * view * vec4(vPosition, 1.0);
})";

const static char* voxelShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec3 vPosition;
in float vBrightness;
in vec2 vTexCoord;
in vec3 vNormal;
in float vDiffuse;

uniform sampler2D diffuse_texture;
uniform float daylight = 1.0;

void main(){
  vec3 color = texture(diffuse_texture, vTexCoord).rgb;
  if(color == vec3(1.0, 0.0, 1.0)){
    discard;
  }

  vec3 normal = normalize(vNormal);

  float ambient = daylight * 0.9;
  vec3 light = vec3(max(max(0.01, daylight * 0.08), ambient * vDiffuse) * vBrightness);

  float f = pow(clamp(gl_FragCoord.z / gl_FragCoord.w / 1000, 0, 0.8), 2);

  FragColor = vec4(mix(color * light, vec3(0.53, 0.81, 0.92) * daylight, f), 1.0);
})";

void processInput(GLFWwindow *window){
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    glfwSetWindowShouldClose(window, true);
  }

  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
    camera.processKeyboard(FORWARD, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
    camera.processKeyboard(BACKWARD, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
    camera.processKeyboard(LEFT, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
    camera.processKeyboard(RIGHT, deltaTime);
  }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos){
  if(firstMouse){
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.processMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
  camera.processMouseScroll(yoffset);
}

int main(int argc, char** argv){
  Window window(WINDOW_WIDTH, WINDOW_HEIGHT, "cppvoxel");

  glfwSetCursorPosCallback(window.window, mouseCallback);
  glfwSetScrollCallback(window.window, scrollCallback);
  glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  Texture texture("tiles.png", GL_NEAREST);

  Shader shader(voxelShaderVertexSource, voxelShaderFragmentSource);
  shader.use();
  shader.setInt("diffuse_texture", 0);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

  Chunk chunk(0, 0, 0);

  glm::mat4 model = glm::mat4(1.0f);
  // model = glm::translate(model, glm::vec3(chunk.x * CHUNK_SIZE, chunk.y * CHUNK_SIZE, chunk.z * CHUNK_SIZE));
  shader.setMat4("model", model);

  while(!window.shouldClose()){
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window.window);

    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(camera.fov), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 1000.0f);
    shader.setMat4("projection", projection);

    glm::mat4 cameraView = camera.getViewMatrix();
    shader.setMat4("view", cameraView);
    shader.setVec3("camera_position", camera.position);

    chunk.update();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    chunk.draw();

    window.pollEvents();
    window.swapBuffers();
  }

  return 0;
}
