#ifndef GL_VAO_H_
#define GL_VAO_H_

#include "gl/utils.h"

#include "common.h"

namespace GL{

enum DataType{
  BYTE = GL_BYTE,
  INT = GL_INT
};

class VAO{
public:
  VAO();
  ~VAO();

  void bind();
  static void unbind();

  template <typename T>
  void attrib(uint index, uint size, DataType type);
  template <typename T>
  void attrib(uint index, uint size, DataType type, uint divisor);

  void attribI(uint index, uint size, DataType type);

private:
  uint handle;
};

}

#endif
