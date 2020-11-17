#ifndef GL_INSTANCE_BUFFER_H_
#define GL_INSTANCE_BUFFER_H_

#include "gl/vao.h"

#include "common.h"

namespace GL {

template <typename T>
class InstanceBuffer {
public:
  InstanceBuffer(VAO* vao, int initialSize, uint attrib);
  ~InstanceBuffer();

  void bind();
  void expand(int newLength);
  void bufferData(int newLength);

private:
  uint vbo;
  size_t vertexSize;
  int currentLength;
};

}

#endif
