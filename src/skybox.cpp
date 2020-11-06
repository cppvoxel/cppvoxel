#include "skybox.h"

#include "common.h"
#include "gl/utils.h"
#include "gl/vao.h"
#include "gl/buffer.h"

#define SKYBOX_SIZE 1

const int8_t vertices[] = {
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE
};

const uint8_t indices[] = {
  2, 1, 0, 0, 3, 2,
  6, 5, 4, 4, 7, 6,
  10, 9, 8, 8, 11, 10,
  14, 13, 12, 12, 15, 14,
  18, 17, 16, 16, 19, 18,
  22, 21, 20, 20, 23, 22
};

namespace Skybox{
  GL::Shader* shader;
  int shaderProjectionLocation, shaderViewLocation;

  GL::VAO* vao;
}

void Skybox::init(){
  shader = new GL::Shader(GL::Shaders::skybox);
  shader->use();
  shaderProjectionLocation = shader->getUniformLocation("projection");
  shaderViewLocation = shader->getUniformLocation("view");

  vao = new GL::VAO();

  vao->bind();

  GL::Buffer<GL::ARRAY>* vbo = new GL::Buffer<GL::ARRAY>();
  vbo->data(sizeof(vertices), vertices);

  GL::Buffer<GL::ELEMENT_ARRAY>* ebo = new GL::Buffer<GL::ELEMENT_ARRAY>();
  ebo->data(sizeof(indices), indices);

  // position
  vao->attrib<int8_t>(0, 3, GL_BYTE);

  GL::VAO::unbind();
  delete vbo;
  delete ebo;
}

void Skybox::free(){
  delete vao;
  delete shader;
}

void Skybox::draw(glm::mat4 projection, glm::mat4 view){
  GL::setDepthTest(GL::LEQUAL);

  shader->use();
  shader->setMat4(shaderProjectionLocation, projection);
  shader->setMat4(shaderViewLocation, view);

  vao->bind();
  GL::drawElements(36);

  GL::setDepthTest(GL::LESS);
}
