#include "skybox.h"

#include "common.h"
#include "gl/utils.h"
#include "gl/vao.h"
#include "gl/buffer.h"

namespace Skybox {
GL::Shader* shader;
int shaderProjectionLocation, shaderViewLocation;

GL::VAO* vao;
}

void Skybox::init() {
  shader = new GL::Shader(GL::Shaders::skybox);
  shader->use();
  shaderProjectionLocation = shader->getUniformLocation("projection");
  shaderViewLocation = shader->getUniformLocation("view");

  vao = new GL::VAO();
  GL::Buffer<GL::ARRAY>* vbo = new GL::Buffer<GL::ARRAY>();

  vao->bind();
  vbo->data(sizeof(cube_vertices), cube_vertices);
  vao->attrib<int8_t>(0, 4, GL::BYTE);

  GL::VAO::unbind();
  delete vbo;
}

void Skybox::free() {
  delete vao;
  delete shader;
}

void Skybox::draw(glm::mat4 projection, glm::mat4 view) {
  GL::setDepthTest(GL::LEQUAL);
  GL::setCullFace(GL::FRONT);

  shader->use();
  shader->setMat4(shaderProjectionLocation, projection);
  shader->setMat4(shaderViewLocation, view);

  vao->bind();
  GL::drawArrays(48);

  GL::setCullFace(GL::BACK);
  GL::setDepthTest(GL::LESS);
}
