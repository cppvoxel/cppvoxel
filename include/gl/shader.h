#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "gl/shader_source.h"
#include "shaders.h"

#include "common.h"

namespace GL{

class Shader{
public:
  Shader(ShaderSource source);
  ~Shader();

  void use();
  
  inline int getUniformLocation(const char* name) const{
    return glGetUniformLocation(id, name);
  }

  // setters

  void setInt(const char* name, int value) const{
    glUniform1i(getUniformLocation(name), value);
  }
  void setInt(int location, int value) const{
    glUniform1i(location, value);
  }

  void setVec3(const char* name, glm::vec3 &value) const{
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
  }
  void setVec3(int location, glm::vec3 &value) const{
    glUniform3fv(location, 1, &value[0]);
  }

  void setMat4(const char* name, glm::mat4 &value) const{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, &value[0][0]);
  }
  void setMat4(int location, glm::mat4 &value) const{
    glUniformMatrix4fv(location, 1, GL_FALSE, &value[0][0]);
  }

private:
  uint id = 0;
};

}

#endif
