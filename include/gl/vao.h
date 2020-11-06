#ifndef GL_VAO_H_
#define GL_VAO_H_

#include "gl/utils.h"

#include "common.h"

namespace GL{

class VAO{
public:
  VAO();
  ~VAO();

  void bind();
  static void unbind();

  template <typename T>
  void attrib(uint index, uint size, GLenum type);
  template <typename T>
  void attrib(uint index, uint size, GLenum type, uint divisor);

  void attribI(uint index, uint size, GLenum type);

private:
  uint handle;
};

}

#endif
