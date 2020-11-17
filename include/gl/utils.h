#ifndef GL_UTILS_H_
#define GL_UTILS_H_

#include <GL/glew.h>

#include "common.h"

namespace GL {

struct ShaderSource {
  const char* vertex;
  const char* fragment;
};

enum DepthTest {
  LESS = GL_LESS,
  LEQUAL = GL_LEQUAL
};

enum String {
  VERSION = GL_VERSION,
  SHADING_LANGUAGE_VERSION = GL_SHADING_LANGUAGE_VERSION,
  RENDERER = GL_RENDERER,
  VENDOR = GL_VENDOR
};

enum Option {
  FRAMEBUFFER_SRGB = GL_FRAMEBUFFER_SRGB,
  DEPTH_TEST = GL_DEPTH_TEST,
  CULL_FACE = GL_CULL_FACE,
  BLEND = GL_BLEND,
  MULTISAMPLE = GL_MULTISAMPLE
};

enum CullFace {
  BACK = GL_BACK,
  FRONT = GL_FRONT
};

enum BlendFunction {
  SRC_ALPHA = GL_SRC_ALPHA,
  ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA
};

enum ClearMask : uint {
  COLOR = GL_COLOR_BUFFER_BIT,
  DEPTH = GL_DEPTH_BUFFER_BIT
};

void init();
template <typename T> T* mapBuffer();
void unmapBuffer();
void drawInstanced(uint verticesCount, uint objectCount);
void drawElements(uint count);
void drawArrays(uint count);
void setDepthTest(DepthTest depthTest);
const uint8_t* getString(String string);
void viewport(int width, int height);
void enable(Option option);
void setCullFace(CullFace face);
void setBlendFunction(BlendFunction a, BlendFunction b);
void setClearColor(float red, float green, float blue, float alpha);
void clear(uint mask);

}

#endif
