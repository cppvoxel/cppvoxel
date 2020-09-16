#define MULTI_THREADING

#include <stdio.h>
#include <signal.h>
#include <limits.h>

#include <thread>

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
#include "chunk.h"
#include "config.h"
#include "skybox.h"

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int windowWidth = 800;
int windowHeight = 600;
int windowedXPos, windowedYPos, windowedWidth, windowedHeight;

Config config("res/config.conf");
Window window(windowWidth, windowHeight, "cppvoxel");

// config
int viewDistance;
int maxChunksGeneratedPerFrame;
int maxChunksDeletedPerFrame;

Camera camera(glm::vec3(0.0f, 50.0f, 0.0f));
float lastX = (float)windowWidth / 2.0f;
float lastY = (float)windowHeight / 2.0f;
bool firstMouse = true;

vec3i pos;
bool fullscreenToggled = false;
bool screenshotToggled = false;
bool perspectiveChanged = true;

glm::mat4 projection = glm::mat4(1.0f);
glm::mat4 cameraView;

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

void main(){
  vTexCoord = texCoord;
  vDiffuse = brightness / 5.0;

  gl_Position = projection * view * model * vec4(coord.xyz, 1.0);
})";

const static char* voxelShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;
in float vDiffuse;

uniform sampler2D diffuse_texture;

const vec3 fogColor = vec3(0.15, 0.3, 0.4);
const float fogDensity = 0.00001;

void main(){
  vec3 color = texture(diffuse_texture, vTexCoord).rgb;
  if(color == vec3(1.0, 0.0, 1.0)){
    discard;
  }

  float z = gl_FragCoord.z / gl_FragCoord.w;
  float fog = clamp(exp(-fogDensity * z * z), 0.2, 1.0);
  FragColor = vec4(mix(fogColor, color * vDiffuse, fog), 1.0);
})";

void signalHandler(int signum){
  fprintf(stderr, "Interrupt signal %d received (pid: %d)\n", signum, getpid());

  exit(signum);  
}

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char *message, const void *userParam){
  // ignore non-significant error/warning codes
  if(id == 131169 || id == 131185 || id == 131218 || id == 131204){
    return; 
  }

  printf("---------------\n");
  printf("debug message (%u): %s\n", id, message);

  switch(source){
    case GL_DEBUG_SOURCE_API:             printf("source: api\n"); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   printf("source: window system\n"); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: printf("source: shader compiler\n"); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     printf("source: third party\n"); break;
    case GL_DEBUG_SOURCE_APPLICATION:     printf("source: application\n"); break;
    case GL_DEBUG_SOURCE_OTHER:           printf("source: other\n"); break;
  }

  switch(type){
    case GL_DEBUG_TYPE_ERROR:               printf("type: error\n"); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("type: deprecated behaviour\n"); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("type: undefined behaviour\n"); break; 
    case GL_DEBUG_TYPE_PORTABILITY:         printf("type: portability\n"); break;
    case GL_DEBUG_TYPE_PERFORMANCE:         printf("type: performance\n"); break;
    case GL_DEBUG_TYPE_MARKER:              printf("type: marker\n"); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          printf("type: push group\n"); break;
    case GL_DEBUG_TYPE_POP_GROUP:           printf("type: pop group\n"); break;
    case GL_DEBUG_TYPE_OTHER:               printf("type: other\n"); break;
  }

  switch(severity){
    case GL_DEBUG_SEVERITY_HIGH:         printf("severity: high\n"); break;
    case GL_DEBUG_SEVERITY_MEDIUM:       printf("severity: medium\n"); break;
    case GL_DEBUG_SEVERITY_LOW:          printf("severity: low\n"); break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: printf("severity: notification\n"); break;
  }
}

void framebufferResizeCallback(GLFWwindow* _window, int width, int height){
  glViewport(0, 0, width, height);
  windowWidth = width;
  windowHeight = height;
  perspectiveChanged = true;
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

  if(!screenshotToggled && glfwGetKey(_window, GLFW_KEY_F2) == GLFW_PRESS){
    screenshotToggled = true;
    printf("saving screenshot...");

    const int pixelsSize = windowWidth * windowHeight * 3;
    uint8_t* pixels = new uint8_t[pixelsSize];

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_FRONT);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

    FILE* file = fopen("screenshot.tga", "w");
    // short header[] = {0, 2, 0, 0, 0, 0, (short)windowWidth, (short)windowHeight, 24};

    // fwrite(&header, sizeof(header), 1, file);

    uint8_t tgaHeader[12] = {0,0,2,0,0,0,0,0,0,0,0,0};
    uint8_t header[6] = {windowWidth%256, windowWidth/256, windowHeight%256, windowHeight/256, 24,0};

    fwrite(tgaHeader, sizeof(uint8_t), 12, file);
    fwrite(header, sizeof(uint8_t), 6, file);
    fwrite(pixels, sizeof(uint8_t), pixelsSize, file);
    fclose(file);

    delete[] pixels;
    printf("screenshot saved");
  }else if(glfwGetKey(_window, GLFW_KEY_F11) == GLFW_RELEASE){
    screenshotToggled = false;
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
    lastX = (float)xpos;
    lastY = (float)ypos;
    firstMouse = false;
  }

  float xoffset = (float)xpos - lastX;
  float yoffset = lastY - (float)ypos;

  lastX = (float)xpos;
  lastY = (float)ypos;

  camera.processMouseMovement(xoffset, yoffset);
}

void scrollCallback(GLFWwindow* _window, double xoffset, double yoffset){
  camera.processMouseScroll((float)yoffset);
}

inline bool isChunkInsideFrustum(glm::mat4 model){
  glm::mat4 mvp = projection * cameraView * model;
  glm::vec4 center = mvp * glm::vec4(CHUNK_SIZE / 2, CHUNK_SIZE / 2, CHUNK_SIZE / 2, 1);
  center.x /= center.w;
  center.y /= center.w;

  return !(center.z < -CHUNK_SIZE / 2 || fabsf(center.x) > 1 + fabsf(CHUNK_SIZE * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CHUNK_SIZE * 2 / center.w));
}

void updateChunks(){
  // delete chunks
  unsigned short chunksDeleted = 0;
  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    Chunk* chunk = it->second;

    int dx = pos.x - chunk->x;
    int dy = pos.y - chunk->y;
    int dz = pos.z - chunk->z;

    // delete chunks outside of render radius
    if(abs(dx) > viewDistance || abs(dy) > viewDistance || abs(dz) > viewDistance){
      it = chunks.erase(it);
      delete chunk;

      chunksDeleted++;
      if(chunksDeleted >= maxChunksDeletedPerFrame){
        break;
      }
    }
  }

  // double start = glfwGetTime();
  // generate new chunks needed
  vec3i chunkPos;
  Chunk* chunk;
  unsigned short chunksGenerated = 0;
  for(int i = -viewDistance; i <= viewDistance && chunksGenerated < maxChunksGeneratedPerFrame; i++){
    for(int j = -viewDistance; j <= viewDistance && chunksGenerated < maxChunksGeneratedPerFrame; j++){
      for(int k = -viewDistance; k <= viewDistance && chunksGenerated < maxChunksGeneratedPerFrame; k++){
        chunkPos.x = pos.x + i;
        chunkPos.y = pos.y + k;
        chunkPos.z = pos.z + j;

        if(getChunk(chunkPos) != NULL){
          continue;
        }

        chunk = new Chunk(chunkPos.x, chunkPos.y, chunkPos.z);
        chunks[chunkPos] = chunk;

        if(chunk->changed){
          chunksGenerated++;
        }
      }
    }
  }
  // printf("chunk generate %.2fms\n", (glfwGetTime() - start) * 1000.0);

  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    if(isChunkInsideFrustum(it->second->model)){
      it->second->update();
    }
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
  signal(SIGABRT, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGILL, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGSEGV, signalHandler);
  signal(SIGTERM, signalHandler);

  printf("== Config ==\n");
  viewDistance = config.getInt("viewDistance", 8);
  maxChunksGeneratedPerFrame = config.getInt("maxChunksGeneratedPerFrame", 32);
  maxChunksDeletedPerFrame = config.getInt("maxChunksDeletedPerFrame", 64);
  bool vsync = config.getBool("vsync", false);

#ifdef MULTI_THREADING
  printf("multithreading enabled\n");
#else
  printf("multithreading disabled\n");
#endif

  printf("== OpenGL ==\n");
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("shading language version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("vendor: %s\n", glGetString(GL_VENDOR));

  printf("== System ==\n");
  printf("CHAR_MAX  : %d\n", CHAR_MAX);
  printf("CHAR_MIN  : %d\n", CHAR_MIN);
  printf("INT_MAX   : %d\n", INT_MAX);
  printf("INT_MIN   : %d\n", INT_MIN);
  printf("SCHAR_MAX : %d\n", SCHAR_MAX);
  printf("SCHAR_MIN : %d\n", SCHAR_MIN);
  printf("SHRT_MAX  : %d\n", SHRT_MAX);
  printf("SHRT_MIN  : %d\n", SHRT_MIN);
  printf("UCHAR_MAX : %d\n", UCHAR_MAX);
  printf("UINT_MAX  : %u\n", (unsigned int)UINT_MAX);
  printf("USHRT_MAX : %d\n", (unsigned short)USHRT_MAX);

  glfwSetFramebufferSizeCallback(window.window, framebufferResizeCallback);
  glfwSetCursorPosCallback(window.window, mouseCallback);
  glfwSetScrollCallback(window.window, scrollCallback);

  glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSwapInterval(vsync ? 1 : 0);

  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if(flags & GL_CONTEXT_FLAG_DEBUG_BIT){
    printf("OpenGL debug supported\n");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); 
    glDebugMessageCallback(glDebugOutput, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
  }else{
    printf("OpenGL debug not supported\n");
  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.2f, 0.6f, 0.8f, 1.0f);

  Texture texture("tiles.png", GL_NEAREST);

  createSkybox();

  Shader shader(voxelShaderVertexSource, voxelShaderFragmentSource);
  shader.use();
  shader.setInt("diffuse_texture", 0);

  pos.x = (int)floorf(camera.position.x / CHUNK_SIZE);
  pos.y = (int)floorf(camera.position.y / CHUNK_SIZE);
  pos.z = (int)floorf(camera.position.z / CHUNK_SIZE);

#ifdef MULTI_THREADING
  std::thread chunkThread(updateChunksThread);
#endif

  float currentTime;
  unsigned int elements = 0;
  unsigned int chunksDrawn =0;

  int dx;
  int dy;
  int dz;

  unsigned short frames = 0;
  float lastPrintTime = (float)glfwGetTime();

  while(!window.shouldClose()){
    currentTime = (float)glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    frames++;

    if(currentTime - lastPrintTime >= 1.0f){
      printf("%.2fms (%dfps) %d chunks (%u elements, %u chunks drawn)\n", 1000.0f/(float)frames, frames, (int)chunks.size(), elements, chunksDrawn);
      frames = 0;
      lastPrintTime += 1.0f;
    }

    processInput(window.window);

    elements = 0;
    chunksDrawn = 0;

    shader.use();

    if(perspectiveChanged){
      projection = glm::perspective(glm::radians(camera.fov), (float)windowWidth/(float)windowHeight, .1f, SKYBOX_SIZE * 2.0f);
      shader.setMat4("projection", projection);
    }

    cameraView = camera.getViewMatrix();
    shader.setMat4("view", cameraView);

    pos.x = (int)floorf(camera.position.x / CHUNK_SIZE);
    pos.y = (int)floorf(camera.position.y / CHUNK_SIZE);
    pos.z = (int)floorf(camera.position.z / CHUNK_SIZE);

#ifndef MULTI_THREADING
    updateChunks();
#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // double start = glfwGetTime();
    for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
      Chunk* chunk = it->second;
      // don't draw if chunk has no mesh
      if(chunk == NULL || !chunk->elements){
        continue;
      }

      dx = pos.x - chunk->x;
      dy = pos.y - chunk->y;
      dz = pos.z - chunk->z;

      // don't render chunks outside of render radius
      if(abs(dx) > viewDistance || abs(dy) > viewDistance || abs(dz) > viewDistance){
        continue;
      }

      if(!isChunkInsideFrustum(chunk->model)){
        continue;
      }

      shader.setMat4("model", chunk->model);
      chunk->draw();

      elements += chunk->elements;
      chunksDrawn++;
    }
    // printf("draw all chunks %.4fms\n", (glfwGetTime() - start) * 1000.0);

    skyboxShader->use();
    skyboxShader->setMat4("view", cameraView);

    if(perspectiveChanged){
      skyboxShader->setMat4("projection", projection);
      perspectiveChanged = false;
    }

    drawSkybox();

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

  freeSkybox();

  return 0;
}
