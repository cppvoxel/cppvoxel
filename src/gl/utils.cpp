#include "gl/utils.h"

#include <stdio.h>

#include <glm/gtc/matrix_transform.hpp>

template <typename T> T* GL::mapBuffer(){
  return (T*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
}

void GL::init(){
  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    exit(-1);
  }
}

void GL::unmapBuffer(){
  glUnmapBuffer(GL_ARRAY_BUFFER);
}

void GL::drawInstanced(uint verticesCount, uint objectCount){
  glDrawArraysInstanced(GL_TRIANGLES, 0, verticesCount, objectCount);
}

void GL::drawElements(uint count){
  glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_BYTE, 0);
}

void GL::drawArrays(uint count){
  glDrawArrays(GL_TRIANGLES, 0, count);
}

void GL::setDepthTest(DepthTest depthTest){
  glDepthFunc(depthTest);
}

const uint8_t* GL::getString(String string){
  return glGetString(string);
}

void GL::viewport(int width, int height){
  glViewport(0, 0, width, height);
}

void GL::enable(Option option){
  glEnable(option);
}

void GL::setCullFace(CullFace face){
  glCullFace(face);
}

void GL::setBlendFunction(BlendFunction a, BlendFunction b){
  glBlendFunc(a, b);
}

void GL::setClearColor(float red, float green, float blue, float alpha){
  glClearColor(red, green, blue, alpha);
}

void GL::clear(uint mask){
  glClear(mask);
}

// tell the compiler we will be using these data types
template uint* GL::mapBuffer<uint>();
template glm::mat4* GL::mapBuffer<glm::mat4>();
