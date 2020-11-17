#ifndef GL_BUFFER_H_
#define GL_BUFFER_H_

#include "gl/utils.h"

#include "common.h"

namespace GL {

enum BufferType {
  ARRAY = GL_ARRAY_BUFFER,
  ELEMENT_ARRAY = GL_ELEMENT_ARRAY_BUFFER
};

template <BufferType T>
class Buffer {
public:
  Buffer();
  ~Buffer();

  void bind();
  static void unbind();
  void data(size_t size, const void* data);

private:
  uint handle;
};

}

#endif
