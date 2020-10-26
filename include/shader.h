#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Shader{
public:
  Shader(const char* vertexSource, const char* fragmentSource);
  ~Shader();

  void use();

  // setters
  void setInt(const char* name, int value) const{
    glUniform1i(glGetUniformLocation(id, name), value);
  }
  void setVec3(const char* name, glm::vec3 &value) const{
    glUniform3fv(glGetUniformLocation(id, name), 1, &value[0]);
  }
  void setMat4(const char* name, glm::mat4 &value) const{
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, &value[0][0]);
  }

private:
  unsigned int id = 0;
};

#endif
