#include "gl/texture_array.h"

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

GL::TextureArray::TextureArray(uint index, uint textureCount, uint textureRes, uint8_t* loadTexture(int)){
  glActiveTexture(GL_TEXTURE0 + index);

  glGenTextures(1, &handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB_ALPHA, textureRes, textureRes, textureCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  stbi_set_flip_vertically_on_load(true);

  uint8_t* imageData;
  for(int i = 0; i < (int)textureCount; i++){
    imageData = loadTexture(i);
    if(imageData){
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, textureRes, textureRes, 1, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    }else{
      fprintf(stderr, "error loading texture\n");
      exit(-1);
    }

    stbi_image_free(imageData);
  }

  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

GL::TextureArray::~TextureArray(){
  glDeleteTextures(1, &handle);
}
