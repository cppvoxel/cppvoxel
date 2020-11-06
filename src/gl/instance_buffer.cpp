#include "gl/instance_buffer.h"

#include <stdio.h>
#include <math.h>
#include <typeinfo>

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

#include "common.h"

// reference: https://github.com/Vercidium/particles/blob/master/source/InstanceBuffer.cs

namespace GL{

template <typename T>
InstanceBuffer<T>::InstanceBuffer(VAO* vao, int initialSize, uint attrib){
  glGenBuffers(1, &vbo);
  vao->bind();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  if(typeid(T) == typeid(uint)){
    vertexSize = sizeof(uint);
    glEnableVertexAttribArray(attrib);
    // uint is treated as 4 bytes so it can be used as a color in the shader
    glVertexAttribPointer(attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, vertexSize, (void*)0);
    // must set attribute divisor
    glVertexAttribDivisor(attrib, 1);
  }else if(typeid(T) == typeid(glm::mat4)){
    vertexSize = sizeof(glm::mat4);
    for(uint i = 0; i < 4; i++){
      glEnableVertexAttribArray(attrib + i);
      // mat4 is treated as 4 sets of 4 floats in the shader
      glVertexAttribPointer(attrib + i, 4, GL_FLOAT, GL_FALSE, vertexSize, (void*)(sizeof(float) * 4 * i));
      // must set attrivute divisor
      glVertexAttribDivisor(attrib + i, 1);
    }
  }else{
    fprintf(stderr, "unknown InstanceBuffer data type\n");
    exit(-1);
  }

  bufferData(initialSize);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template <typename T>
InstanceBuffer<T>::~InstanceBuffer(){
  glDeleteBuffers(1, &vbo);
}

template <typename T>
inline void InstanceBuffer<T>::bind(){
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
}

template <typename T>
inline void InstanceBuffer<T>::expand(int newLength){
  if(newLength < currentLength){
    return;
  }

  newLength = MAX(newLength, currentLength * 2); // make sure we only increase by a sizeable amount instead of lots of small increases
  bufferData(newLength);
}

template <typename T>
inline void InstanceBuffer<T>::bufferData(int newLength){
  currentLength = newLength;

  bind();
  glBufferData(GL_ARRAY_BUFFER, (uint)(currentLength * vertexSize), (void*)0, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// tell the compiler we will be using these data types
template class InstanceBuffer<uint>;
template class InstanceBuffer<glm::mat4>;

}