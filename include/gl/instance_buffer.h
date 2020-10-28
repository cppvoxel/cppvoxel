#ifndef INSTANCE_BUFFER_H_
#define INSTANCE_BUFFER_H_

#include <cstdint>

namespace GL{

template <typename T>
class InstanceBuffer{
public:
  InstanceBuffer(unsigned int vao, int initialSize, unsigned int attrib);
  ~InstanceBuffer();

  void bind();
  void expand(int newLength);
  void bufferData(int newLength);

private:
  unsigned int vbo;
  size_t vertexSize;
  int currentLength;
};

}

#endif
