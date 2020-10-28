#ifndef SKYBOX_H_
#define SKYBOX_H_

#include "gl/shader.h"

#define SKYBOX_SIZE 1

namespace Skybox{

extern GL::Shader* shader;

void init();
void draw();
void free();

};

#endif
