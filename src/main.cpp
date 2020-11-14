// #define MULTI_THREADING

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <thread>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "common.h"
#include "config.h"
#include "camera.h"
#include "chunk_manager.h"
#include "chunk.h"
#include "skybox.h"
#include "particle_manager.h"

#include "gl/utils.h"
#include "gl/texture_array.h"

#include "glfw/glfw.h"
#include "glfw/window.h"
#include "glfw/input.h"

// textures
#include "res/dirt.h"
#include "res/stone.h"
#include "res/bedrock.h"
#include "res/sand.h"
#include "res/grass_side.h"
#include "res/glass.h"
#include "res/snow.h"
#include "res/water.h"
#include "res/grass.h"
#include "res/log.h"
#include "res/log_top.h"

#ifndef GIT_BRANCH
#define GIT_BRANCH "unknown"
#endif

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

#define REACH_DISTANCE 20.0f

struct allocation_metrics_t{
  uint totalAllocations = 0;
};
static allocation_metrics_t s_AllocationMetrics;

void* operator new(size_t size){
  s_AllocationMetrics.totalAllocations++;
  return malloc(size);
}

void operator delete(void* memory){
  s_AllocationMetrics.totalAllocations--;
  free(memory);
}

double deltaTime;
double lastFrame;

int windowWidth = 800;
int windowHeight = 600;
int windowedXPos, windowedYPos, windowedWidth, windowedHeight;

GLFW::Window window(windowWidth, windowHeight, "cppvoxel");

// config
int viewDistance;
int maxChunksGeneratedPerFrame;
int maxChunksDeletedPerFrame;

Camera camera(glm::vec3(0.0f, 150.0f, 0.0f));
float lastX = (float)windowWidth / 2.0f;
float lastY = (float)windowHeight / 2.0f;
bool firstMouse = true;

vec3i pos;
bool cursorLocked = true;

glm::mat4 projection = glm::mat4(1.0f);
glm::mat4 cameraView;

void signalHandler(int signum){
  fprintf(stderr, "Interrupt signal %d received (pid: %d)\n", signum, getpid());
  fprintf(stderr, "%s:%u (%s): %s", stackTraceFile.c_str(), stackTraceLine, stackTraceFile.c_str(), stackTraceName.c_str());

  exit(signum);  
}

#ifdef DEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, uint id, GLenum severity, GLsizei length, const char *message, const void *userParam){
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
#endif

void mouseCallback(double xpos, double ypos){
  if(!cursorLocked){
    return;
  }

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

#ifdef MULTI_THREADING
void updateChunksThread(){
  while(!window.shouldClose()){
    ChunkManager::update(pos, viewDistance + 1);

#ifdef _WIN32
    Sleep(0);
#else
    sleep(0);
#endif
  }
}
#endif

/* try 31?
  if 'infinite' is 2 ^ 32
  start your map at 2 ^ 32 / 2

  - Verc
*/ 
bool VercidiumRayMarch(int *bx, int *by, int *bz, int *cx, int *cy, int *cz){
  float vx = camera.front.x;
  float vy = camera.front.y;
  float vz = camera.front.z;

  float t = 0.0f;

  while(t < REACH_DISTANCE){
    float rayx = camera.position.x + vx * t;
    float rayy = camera.position.y + vy * t;
    float rayz = camera.position.z + vz * t;

    std::shared_ptr<Chunk> chunk = ChunkManager::get({(int)floorf(rayx / CHUNK_SIZE), (int)floorf(rayy / CHUNK_SIZE), (int)floorf(rayz / CHUNK_SIZE)});
    if(chunk != nullptr){
      int nx = abs(roundf(rayx));
      int ny = abs(roundf(rayy));
      int nz = abs(roundf(rayz));

      block_t block = chunk->get(nx % CHUNK_SIZE, ny % CHUNK_SIZE, nz % CHUNK_SIZE);
      if(block > 0){
        // printf("chunk: %d %d %d\n", chunk->x, chunk->y, chunk->z);
        // printf("hit block %d at %d %d %d\n", block, nx % CHUNK_SIZE, ny % CHUNK_SIZE, nz % CHUNK_SIZE);
        *bx = nx;
        *by = ny;
        *bz = nz;
        *cx = chunk->x;
        *cy = chunk->y;
        *cz = chunk->z;

        return true;
      }
    }

    t += 0.01f;
  }

  return false;
}

uint8_t* loadTexture(int index){
  int width, height, nrChannels;
  uint8_t* imageData;
  printf("%d;", index);

  switch(index){
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
      break;
    case 10:
      imageData = stbi_load_from_memory(IMAGE_LOG_TOP_BYTES, sizeof(IMAGE_LOG_TOP_BYTES), &width, &height, &nrChannels, STBI_rgb_alpha);
      break;
    default:
      fprintf(stderr, "f");
      exit(-1);
      break;
  }

  return imageData;
}

int main(int argc, char** argv){
  signal(SIGABRT, signalHandler);
  signal(SIGFPE, signalHandler);
  signal(SIGILL, signalHandler);
  signal(SIGINT, signalHandler);
  signal(SIGSEGV, signalHandler);
  signal(SIGTERM, signalHandler);

  STACK_TRACE_PUSH("init")

  printf("== cppvoxel ==\n");
  printf("version: %s@%s\n", GIT_BRANCH, GIT_HASH);

#ifdef MULTI_THREADING
  printf("multithreading: enabled\n");
#else
  printf("multithreading: disabled\n");
#endif

  Config config("config.conf");

  printf("== Config ==\n");
  viewDistance = config.getInt("viewDistance", 6);
  maxChunksGeneratedPerFrame = config.getInt("maxChunksGeneratedPerFrame", 8);
  maxChunksDeletedPerFrame = config.getInt("maxChunksDeletedPerFrame", 16);
  bool vsync = config.getBool("vsync", false);

  printf("== OpenGL ==\n");
  printf("version: %s\n", GL::getString(GL::VERSION));
  printf("shading language version: %s\n", GL::getString(GL::SHADING_LANGUAGE_VERSION));
  printf("renderer: %s\n", GL::getString(GL::RENDERER));
  printf("vendor: %s\n", GL::getString(GL::VENDOR));

  // printf("== System ==\n");
  // printf("double : %lu\n", (long unsigned)sizeof(double));
  // printf("float  : %lu\n", (long unsigned)sizeof(float));
  // printf("int    : %lu\n", (long unsigned)sizeof(int));
  // printf("uint   : %lu\n", (long unsigned)sizeof(uint));
  // printf("short  : %lu\n", (long unsigned)sizeof(short));
  // printf("ushort : %lu\n", (long unsigned)sizeof(ushort));
  // printf("byte   : %lu\n", (long unsigned)sizeof(int8_t));
  // printf("ubyte  : %lu\n", (long unsigned)sizeof(uint8_t));

  window.setMouseCallback(mouseCallback);

  window.setCursorMode(GLFW::DISABLED);
  GLFW::enableVsync(vsync);

  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

#ifdef DEBUG
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
#endif

  CATCH_OPENGL_ERROR

  GL::enable(GL::FRAMEBUFFER_SRGB); 
  GL::enable(GL::DEPTH_TEST);
  GL::enable(GL::CULL_FACE);
  GL::setCullFace(GL::BACK);
  GL::setBlendFunction(GL::SRC_ALPHA, GL::ONE_MINUS_SRC_ALPHA);
  GL::enable(GL::BLEND);
  GL::enable(GL::MULTISAMPLE);
  GL::setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  const int NUM_TEXTURES = 11;
  const int TEXTURE_RES = 16;

  printf("loading textures: ");

  GL::TextureArray* textureArray = new GL::TextureArray(0, NUM_TEXTURES, TEXTURE_RES, loadTexture);

  printf(" done!\n");

  Skybox::init();
  ChunkManager::init();
  ParticleManager::init();

  CATCH_OPENGL_ERROR

  pos.x = (int)floorf(camera.position.x / CHUNK_SIZE);
  pos.y = (int)floorf(camera.position.y / CHUNK_SIZE);
  pos.z = (int)floorf(camera.position.z / CHUNK_SIZE);

#ifdef MULTI_THREADING
  std::thread chunkThread(updateChunksThread);
#endif

  double currentTime;

  unsigned short frames = 0;
  double lastPrintTime = GLFW::getTime();

  STACK_TRACE_PUSH("main loop")

  while(!window.shouldClose()){
    currentTime = GLFW::getTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    frames++;

    if(currentTime - lastPrintTime >= 1.0){
      printf("%.2fms (%dfps) %u chunks %u particles allocated %u allocations\n", 1000.0f/(float)frames, frames, (uint)ChunkManager::chunks.size(), (uint)ParticleManager::particles.size(), s_AllocationMetrics.totalAllocations);
      frames = 0;
      lastPrintTime += 1.0;
    }

    if(Input::getKey(Input::Key::F12).pressed){
      window.setShouldClose(true);
    }

    if(Input::getKey(Input::Key::ESCAPE).pressed){
      if(cursorLocked){
        window.setCursorMode(GLFW::NORMAL);
        firstMouse = true;
      }else{
        window.setCursorMode(GLFW::DISABLED);
      }

      cursorLocked = !cursorLocked;
    }

    if(Input::getKey(Input::Key::F11).pressed){
      window.setFullscreen(!window.getFullscreen());
    }

    if(Input::getKey(Input::Key::F10).pressed){
      vsync = !vsync;
      GLFW::enableVsync(vsync);
    }

    camera.fast = Input::getKey(Input::Key::LEFT_SHIFT).down;
    if(Input::getKey(Input::Key::W).down){
      camera.processKeyboard(FORWARD, deltaTime);
    }else if(Input::getKey(Input::Key::S).down){
      camera.processKeyboard(BACKWARD, deltaTime);
    }
    if(Input::getKey(Input::Key::A).down){
      camera.processKeyboard(LEFT, deltaTime);
    }else if(Input::getKey(Input::Key::D).down){
      camera.processKeyboard(RIGHT, deltaTime);
    }

    bool leftMouse = Input::getMosue(Input::MouseButton::LEFT);
    bool rightMouse = Input::getMosue(Input::MouseButton::RIGHT);

    if(leftMouse || rightMouse){
      int hx, hy, hz, cx, cy, cz;
      if(VercidiumRayMarch(&hx, &hy, &hz, &cx, &cy, &cz)){
        std::shared_ptr<Chunk> chunk = ChunkManager::get({cx, cy, cz});
        if(chunk != NULL){
          chunk->set(hx % CHUNK_SIZE, hy % CHUNK_SIZE, hz % CHUNK_SIZE, leftMouse ? 0 : 1);
        }
      }
    }

    pos.x = (int)floorf(camera.position.x / CHUNK_SIZE);
    pos.y = (int)floorf(camera.position.y / CHUNK_SIZE);
    pos.z = (int)floorf(camera.position.z / CHUNK_SIZE);

#ifndef MULTI_THREADING
    ChunkManager::update(pos);
#endif
    ParticleManager::update(deltaTime, camera.position);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    window.getSize(&windowWidth, &windowHeight);
    projection = glm::perspective(glm::radians(camera.fov), (float)windowWidth/(float)windowHeight, .1f, 10000.0f);
    cameraView = camera.getViewMatrix();

    Skybox::draw(projection, cameraView);
    ChunkManager::draw(projection, cameraView);
    ParticleManager::draw(projection, cameraView);

    CATCH_OPENGL_ERROR

    Input::update();
    window.pollEvents();
    window.swapBuffers();
  }

#ifdef MULTI_THREADING
  chunkThread.join();
#endif

  ParticleManager::free();
  ChunkManager::free();
  Skybox::free();

  delete textureArray;

  return 0;
}
