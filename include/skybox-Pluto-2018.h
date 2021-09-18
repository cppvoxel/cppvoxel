#ifndef SKYBOX_H_
#define SKYBOX_H_

#include "gl/shader.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Skybox {

extern GL::Shader* shader;

void init();
void free();
void draw(glm::mat4 projection, glm::mat4 view);

};

#endif
