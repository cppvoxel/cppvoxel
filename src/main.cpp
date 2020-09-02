#define MULTI_THREADING

#include <stdio.h>
#ifdef MULTI_THREADING
#include <thread>
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Window.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#include "common.h"
#include "Chunk.h"

#define MAX_CHUNKS_GENERATED_PER_FRAME 4
#define MAX_CHUNKS_DELETED_PER_FRAME 32
#define CHUNK_RENDER_RADIUS 8

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int windowWidth = 800;
int windowHeight = 600;
int windowedXPos, windowedYPos, windowedWidth, windowedHeight;

Window window(windowWidth, windowHeight, "cppvoxel");

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)windowWidth / 2.0f;
float lastY = (float)windowHeight / 2.0f;
bool firstMouse = true;

vec3i pos;
vec3i lastPos;
bool fullscreenToggled = false;

const static char* voxelShaderVertexSource = R"(#version 330 core
layout (location = 0) in vec4 coord;
layout (location = 1) in int brightness;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

out vec2 vTexCoord;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

#define LIGHT_DIRECTION normalize(vec3(-0.8, 0.7, 0.3))

void main(){
  vTexCoord = texCoord;
  vec3 norm = mat3(transpose(inverse(model))) * normal;
  vDiffuse = max(0.6, max(0.0, dot(norm, LIGHT_DIRECTION))) * float(brightness) * 0.1 + 1.0;

  gl_Position = projection * view * model * vec4(coord.xyz, 1.0);
})";

const static char* voxelShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
in float vDiffuse;

uniform sampler2D diffuse_texture;

void main(){
  vec3 color = texture(diffuse_texture, vTexCoord).rgb;
  if(color == vec3(1.0, 0.0, 1.0)){
    discard;
  }

  FragColor = vec4(color * vec3(vDiffuse), 1.0);
})";

void framebufferResizeCallback(GLFWwindow* _window, int width, int height){
  glViewport(0, 0, width, height);
  windowWidth = width;
  windowHeight = height;
}

void processInput(GLFWwindow* _window){
  if(glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    glfwSetWindowShouldClose(_window, true);
  }

  if(!fullscreenToggled && glfwGetKey(_window, GLFW_KEY_F11) == GLFW_PRESS){
    fullscreenToggled = true;

    if(glfwGetWindowMonitor(_window)){
      glfwSetWindowMonitor(_window, NULL, windowedXPos, windowedYPos, windowedWidth, windowedHeight, 0);
    }else{
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      if(monitor){
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwGetWindowPos(_window, &windowedXPos, &windowedYPos);
        glfwGetWindowSize(_window, &windowedWidth, &windowedHeight);
        glfwSetWindowMonitor(_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
      }
    }
  }else if(glfwGetKey(_window, GLFW_KEY_F11) == GLFW_RELEASE){
    fullscreenToggled = false;
  }

  camera.fast = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

  if(glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS){
    camera.processKeyboard(FORWARD, deltaTime);
  }
  if(glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS){
    camera.processKeyboard(BACKWARD, deltaTime);
  }
  if(glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS){
    camera.processKeyboard(LEFT, deltaTime);
  }
  if(glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS){
    camera.processKeyboard(RIGHT, deltaTime);
  }
}

void mouseCallback(GLFWwindow* _window, double xpos, double ypos){
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

void scrollCallback(GLFWwindow* _window, double xoffset, double yoffset){
  camera.processMouseScroll(yoffset);
}

void updateChunks(){
  // delete chunks
  unsigned short chunksDeleted = 0;
  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    Chunk* chunk = it->second;

    int dx = lastPos.x - chunk->x;
    int dy = lastPos.y - chunk->y;
    int dz = lastPos.z - chunk->z;

    // delete chunks outside of render radius
    if(abs(dx) > CHUNK_RENDER_RADIUS || abs(dy) > CHUNK_RENDER_RADIUS || abs(dz) > CHUNK_RENDER_RADIUS){
      it = chunks.erase(it);
      delete chunk;

      chunksDeleted++;
      if(chunksDeleted >= MAX_CHUNKS_DELETED_PER_FRAME){
        break;
      }
    }
  }

  // double start = glfwGetTime();
  // generate new chunks needed
  int cx, cy, cz;
  vec3i chunkPos;
  Chunk* chunk;
  unsigned short chunksGenerated = 0;
  for(int8_t i = -CHUNK_RENDER_RADIUS; i <= CHUNK_RENDER_RADIUS && chunksGenerated < MAX_CHUNKS_GENERATED_PER_FRAME; i++){
    for(int8_t j = -CHUNK_RENDER_RADIUS; j <= CHUNK_RENDER_RADIUS && chunksGenerated < MAX_CHUNKS_GENERATED_PER_FRAME; j++){
      for(int8_t k = -CHUNK_RENDER_RADIUS; k <= CHUNK_RENDER_RADIUS && chunksGenerated < MAX_CHUNKS_GENERATED_PER_FRAME; k++){
        cx = lastPos.x + i;
        cy = lastPos.y + k;
        cz = lastPos.z + j;

        chunkPos.x = cx;
        chunkPos.y = cy;
        chunkPos.z = cz;

        if(getChunk(chunkPos) != NULL){
          continue;
        }

        chunk = new Chunk(cx, cy, cz);
        chunks[chunkPos] = chunk;

        chunksGenerated++;
      }
    }
  }
  // printf("chunk generate %.2fms\n", (glfwGetTime() - start) * 1000.0);

  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    it->second->update();
  }
}

#ifdef MULTI_THREADING
void updateChunksThread(){
  while(!window.shouldClose()){
  // double start = glfwGetTime();
    updateChunks();
  // printf("chunk thread %.2fms\n", (glfwGetTime() - start) * 1000.0);

#ifdef _WIN32
    Sleep(0);
#else
    sleep(0);
#endif
  }
}
#endif

int main(int argc, char** argv){
  glfwSetFramebufferSizeCallback(window.window, framebufferResizeCallback);
  glfwSetCursorPosCallback(window.window, mouseCallback);
  glfwSetScrollCallback(window.window, scrollCallback);

  glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSwapInterval(0);

  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.2f, 0.6f, 0.8f, 1.0f);

  Texture texture("tiles.png", GL_NEAREST);

  Shader shader(voxelShaderVertexSource, voxelShaderFragmentSource);
  shader.use();
  shader.setInt("diffuse_texture", 0);

  glm::mat4 projection = glm::mat4(1.0f);
  glm::mat4 cameraView;
  glm::mat4 model;

  pos.x = floorf(camera.position.x / CHUNK_SIZE);
  pos.y = floorf(camera.position.y / CHUNK_SIZE);
  pos.z = floorf(camera.position.z / CHUNK_SIZE);

  lastPos.x = pos.x;
  lastPos.y = pos.y;
  lastPos.z = pos.z;

  float lastPrintTime = glfwGetTime();
  unsigned short frames = 0;

#ifdef MULTI_THREADING
  std::thread chunkThread(updateChunksThread);
#endif

  float currentTime;
  while(!window.shouldClose()){
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    frames++;

    if(currentTime - lastPrintTime >= 1.0f){
      printf("%.1fms (%dfps) %d chunks\n", 1000.0f/(float)frames, frames, (int)chunks.size());
      frames = 0;
      lastPrintTime += 1.0f;
    }

    processInput(window.window);

    projection = glm::perspective(glm::radians(camera.fov), (float)windowWidth/(float)windowHeight, 0.1f, 1000.0f);
    shader.setMat4("projection", projection);

    cameraView = camera.getViewMatrix();
    shader.setMat4("view", cameraView);

    pos.x = floorf(camera.position.x / CHUNK_SIZE);
    pos.y = floorf(camera.position.y / CHUNK_SIZE);
    pos.z = floorf(camera.position.z / CHUNK_SIZE);

#ifndef MULTI_THREADING
    updateChunks();
#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int dx;
    int dy;
    int dz;
    for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
      Chunk* chunk = it->second;
      // don't draw if chunk has no mesh
      if(!chunk->elements){
        continue;
      }

      dx = pos.x - chunk->x;
      dy = pos.y - chunk->y;
      dz = pos.z - chunk->z;

      // don't render chunks outside of render radius
      if(abs(dx) > CHUNK_RENDER_RADIUS || abs(dy) > CHUNK_RENDER_RADIUS || abs(dz) > CHUNK_RENDER_RADIUS){
        continue;
      }

      model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(chunk->x * CHUNK_SIZE, chunk->y * CHUNK_SIZE, chunk->z * CHUNK_SIZE));
      shader.setMat4("model", model);

      chunk->draw();
    }

    lastPos.x = pos.x;
    lastPos.y = pos.y;
    lastPos.z = pos.z;

    window.pollEvents();
    window.swapBuffers();
  }

#ifdef MULTI_THREADING
  chunkThread.join();
#endif

  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    Chunk* chunk = it->second;
    delete chunk;
  }

  return 0;
}
