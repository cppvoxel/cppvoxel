#define MULTI_THREADING

#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <thread>

#ifdef MULTI_THREADING
#include <mutex>
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
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Window.h>
#include <Shader.h>
#include <Camera.h>

#include "common.h"
#include "chunk.h"
#include "config.h"
#include "skybox.h"
#include "input.h"

// textures
#include "dirt.h"
#include "stone.h"
#include "bedrock.h"
#include "sand.h"
#include "grass_side.h"
#include "glass.h"
#include "snow.h"
#include "water.h"
#include "grass.h"
#include "log.h"
#include "log_top.h"

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

Camera camera(glm::vec3(0.0f, 150.0f, 0.0f));
float lastX = (float)windowWidth / 2.0f;
float lastY = (float)windowHeight / 2.0f;
bool firstMouse = true;

vec3i pos;
bool perspectiveChanged = true;

glm::mat4 projection = glm::mat4(1.0f);
glm::mat4 cameraView;

#ifdef MULTI_THREADING
std::mutex chunkThreadMutex;
#endif

const static char* voxelShaderVertexSource = R"(#version 330 core
layout (location = 0) in vec4 coord;
// layout (location = 1) in int brightness;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

out vec3 vPosition;
out vec3 vTexCoord;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const vec3 sun_direction = normalize(vec3(1, 3, 2));
const float ambient = 0.4f;

void main(){
  vPosition = (view * model * vec4(coord.xyz, 1.0)).xyz;
  vTexCoord = vec3(texCoord.xy, coord.w);
  vDiffuse = (max(dot(normal, sun_direction), 0.0) + ambient) /** (brightness / 5.0)*/;

  gl_Position = projection * vec4(vPosition, 1.0);
})";

const static char* voxelShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec3 vPosition;
in vec3 vTexCoord;
in float vDiffuse;

uniform sampler2DArray texture_array;

uniform int fog_near;
uniform int fog_far;

void main(){
  FragColor = vec4(texture(texture_array, vTexCoord).rgb * vDiffuse, 1.0);
  FragColor *= 1.0 - smoothstep(fog_near, fog_far, length(vPosition));
  FragColor = vec4(pow(FragColor.rgb, vec3(1.0 / 2.2)), FragColor.a);
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

inline void updateChunks(){
  // double start = glfwGetTime();
  // generate new chunks needed
  vec3i camPos = pos;
  vec3i chunkPos;
  Chunk* chunk;
  // unsigned short chunksGenerated = 0;
  int distance = viewDistance + 1;

  for(int i = -distance; i <= distance; i++){
    for(int j = -distance; j <= distance; j++){
      for(int k = -distance; k <= distance; k++){
        chunkPos.x = camPos.x + i;
        chunkPos.y = camPos.y + k;
        chunkPos.z = camPos.z + j;

        if(getChunk(chunkPos) != NULL){
          continue;
        }

        chunk = new Chunk(chunkPos.x, chunkPos.y, chunkPos.z);
        chunks[chunkPos] = chunk;

        // chunksGenerated++;
      }
    }
  }

  // printf("chunk generate: %u in %.4fms\n", chunksGenerated, (glfwGetTime() - start) * 1000.0);
}

#ifdef MULTI_THREADING
void updateChunksThread(){
  while(!window.shouldClose()){
    updateChunks();

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

  Input::init(window.window);

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

  CATCH_OPENGL_ERROR

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  const int NUM_TEXTURES = 11;
  const int TEXTURE_RES = 16;

  glActiveTexture(GL_TEXTURE0);
  unsigned int textureArray;
  glGenTextures(1, &textureArray);
  glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB_ALPHA, TEXTURE_RES, TEXTURE_RES, NUM_TEXTURES, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  stbi_set_flip_vertically_on_load(true);
  for(int i = 0; i < NUM_TEXTURES; i++){
    int width, height, nrChannels;

    uint8_t* imageData;
    switch(i){
      case 0:
        imageData = stbi_load_from_memory(IMAGE_DIRT_BYTES, sizeof(IMAGE_DIRT_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 1:
        imageData = stbi_load_from_memory(IMAGE_STONE_BYTES, sizeof(IMAGE_STONE_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 2:
        imageData = stbi_load_from_memory(IMAGE_BEDROCK_BYTES, sizeof(IMAGE_BEDROCK_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 3:
        imageData = stbi_load_from_memory(IMAGE_SAND_BYTES, sizeof(IMAGE_SAND_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 4:
        imageData = stbi_load_from_memory(IMAGE_GRASS_SIDE_BYTES, sizeof(IMAGE_GRASS_SIDE_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 5:
        imageData = stbi_load_from_memory(IMAGE_GLASS_BYTES, sizeof(IMAGE_GLASS_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 6:
        imageData = stbi_load_from_memory(IMAGE_SNOW_BYTES, sizeof(IMAGE_SNOW_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 7:
        imageData = stbi_load_from_memory(IMAGE_WATER_BYTES, sizeof(IMAGE_WATER_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 8:
        imageData = stbi_load_from_memory(IMAGE_GRASS_BYTES, sizeof(IMAGE_GRASS_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
      case 9:
        imageData = stbi_load_from_memory(IMAGE_LOG_BYTES, sizeof(IMAGE_LOG_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
      case 10:
        imageData = stbi_load_from_memory(IMAGE_LOG_TOP_BYTES, sizeof(IMAGE_LOG_TOP_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
        break;
        break;
      default:
        fprintf(stderr, "invalid texture id\n");
        exit(-1);
        break;
    }

    if(imageData){
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, TEXTURE_RES, TEXTURE_RES, 1, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
      printf("loaded texture %d\n", i);
    }else{
      fprintf(stderr, "failed to load texture %d\n", i);
    }

    stbi_image_free(imageData);
  }

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  Skybox::create();

  Shader shader(voxelShaderVertexSource, voxelShaderFragmentSource);
  shader.use();
  shader.setInt("texture_array", 0);
  shader.setInt("fog_near", viewDistance * CHUNK_SIZE - 12);
  shader.setInt("fog_far", viewDistance * CHUNK_SIZE - 6);

  CATCH_OPENGL_ERROR

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

    if(Input::getKey(Input::F12).pressed || Input::getKey(Input::ESCAPE).pressed){
      glfwSetWindowShouldClose(window.window, true);
    }

    if(Input::getKey(Input::F11).pressed){
      if(glfwGetWindowMonitor(window.window)){
        glfwSetWindowMonitor(window.window, NULL, windowedXPos, windowedYPos, windowedWidth, windowedHeight, 0);
      }else{
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if(monitor){
          const GLFWvidmode* mode = glfwGetVideoMode(monitor);
          glfwGetWindowPos(window.window, &windowedXPos, &windowedYPos);
          glfwGetWindowSize(window.window, &windowedWidth, &windowedHeight);
          glfwSetWindowMonitor(window.window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
      }
    }

    if(Input::getKey(Input::F2).pressed){
      // FIXME: please
      printf("saving screenshot...\n");

      const int pixelsSize = windowWidth * windowHeight * 3;
      uint8_t* pixels = new uint8_t[pixelsSize];

      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glReadBuffer(GL_FRONT);
      glReadPixels(0, 0, windowWidth, windowHeight, GL_BGR, GL_UNSIGNED_BYTE, pixels);

      FILE* file = fopen("screenshot.tga", "w");
      short header[] = {0, 2, 0, 0, 0, 0, (short)windowWidth, (short)windowHeight, 24};

      fwrite(&header, sizeof(header), 1, file);
      fwrite(pixels, sizeof(uint8_t), pixelsSize, file);
      fclose(file);

      delete[] pixels;
      printf("screenshot saved\n");
    }

    camera.fast = Input::getKey(Input::LEFT_SHIFT).down;
    if(Input::getKey(Input::W).down){
      camera.processKeyboard(FORWARD, deltaTime);
    }
    if(Input::getKey(Input::S).down){
      camera.processKeyboard(BACKWARD, deltaTime);
    }
    if(Input::getKey(Input::A).down){
      camera.processKeyboard(LEFT, deltaTime);
    }
    if(Input::getKey(Input::D).down){
      camera.processKeyboard(RIGHT, deltaTime);
    }

    elements = 0;
    chunksDrawn = 0;

    pos.x = (int)floorf(camera.position.x / CHUNK_SIZE);
    pos.y = (int)floorf(camera.position.y / CHUNK_SIZE);
    pos.z = (int)floorf(camera.position.z / CHUNK_SIZE);

#ifndef MULTI_THREADING
    updateChunks();
#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    cameraView = camera.getViewMatrix();

    Skybox::shader->use();
    Skybox::shader->setMat4("view", cameraView);

    if(perspectiveChanged){
      projection = glm::perspective(glm::radians(camera.fov), (float)windowWidth/(float)windowHeight, .1f, SKYBOX_SIZE * 2.0f);
      Skybox::shader->setMat4("projection", projection);
    }

    Skybox::draw(); CATCH_OPENGL_ERROR

    shader.use();
    if(perspectiveChanged){
      shader.setMat4("projection", projection);
      perspectiveChanged = false;
    }

    shader.setMat4("view", cameraView);

    unsigned short chunksDeleted = 0;
    unsigned short chunksGenerated = 0;
    // double start = glfwGetTime();
#ifdef MULTI_THREADING
    // chunkThreadMutex.lock();
#endif
    for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
      Chunk* chunk = it->second;

      // FIXME: this should not be needed
      if(chunk == NULL){
        continue;
      }

      dx = pos.x - chunk->x;
      dy = pos.y - chunk->y;
      dz = pos.z - chunk->z;

      // don't render chunks outside of render radius
      if(abs(dx) > viewDistance + 1 || abs(dy) > viewDistance + 1 || abs(dz) > viewDistance + 1){
        if(chunksDeleted < maxChunksDeletedPerFrame){
          it = chunks.erase(it);
          delete chunk;
          chunksDeleted++;
        }

        continue;
      }

      if(chunk->empty || abs(dx) > viewDistance || abs(dy) > viewDistance || abs(dz) > viewDistance || !isChunkInsideFrustum(chunk->model)){
        continue;
      }

      if(chunksGenerated < maxChunksGeneratedPerFrame && chunk->changed){
        chunksGenerated += chunk->update();
      }

      // don't draw if chunk has no mesh
      if(!chunk->elements){
        continue;
      }

      shader.setMat4("model", chunk->model);
      chunk->draw(); CATCH_OPENGL_ERROR

      elements += chunk->elements;
      chunksDrawn++;
    }
#ifdef MULTI_THREADING
    // chunkThreadMutex.unlock();
#endif
    // printf("draw all chunks %.4fms\n", (glfwGetTime() - start) * 1000.0);

    Input::update();
    window.pollEvents();
    window.swapBuffers();
    CATCH_OPENGL_ERROR
  }

#ifdef MULTI_THREADING
  chunkThread.join();
#endif

  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    Chunk* chunk = it->second;
    delete chunk;
  }

  Skybox::free();

  glDeleteTextures(1, &textureArray);

  return 0;
}
