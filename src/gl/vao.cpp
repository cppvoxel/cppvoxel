#include "gl/vao.h"

GL::VAO::VAO(){
  glGenVertexArrays(1, &handle);
}

GL::VAO::~VAO(){
  glDeleteVertexArrays(1, &handle);
}

void GL::VAO::bind(){
  glBindVertexArray(handle);
}

void GL::VAO::unbind(){
  glBindVertexArray(0);
}

template <typename T>
void GL::VAO::attrib(uint index, uint size, DataType type){
  glVertexAttribPointer(index, size, type, GL_FALSE, size * sizeof(T), (void*)0);
  glEnableVertexAttribArray(index);
}

template <typename T>
void GL::VAO::attrib(uint index, uint size, DataType type, uint divisor){
  glVertexAttribPointer(index, size, type, GL_FALSE, size * sizeof(T), (void*)0);
  glEnableVertexAttribArray(index);
  glVertexAttribDivisor(index, divisor);
}

void GL::VAO::attribI(uint index, uint size, DataType type){
  glVertexAttribIPointer(index, size, type, 0, (void*)0);
  glEnableVertexAttribArray(index);
}

template void GL::VAO::attrib<int8_t>(uint, uint, DataType);
template void GL::VAO::attrib<int8_t>(uint, uint, DataType, uint);
