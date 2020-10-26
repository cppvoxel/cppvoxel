#ifndef SKYBOX_H_
#define SKYBOX_H_

#include <Shader.h>

#define SKYBOX_SIZE 1

namespace Skybox{

extern Shader* shader;

void init();
void draw();
void free();

};

#endif
