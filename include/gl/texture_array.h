#ifndef GL_TEXTURE_ARRAY_H_
#define GL_TEXTURE_ARRAY_H_

#include "gl/utils.h"

#include "common.h"

namespace GL{

class TextureArray{
public:
  TextureArray(uint index, uint textureCount, uint textureRes, uint8_t* loadTexture(int));
  ~TextureArray();

private:
  uint handle;
};

}

#endif
