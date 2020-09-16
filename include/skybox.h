#ifndef SKYBOX_H_
#define SKYBOX_H_

#include <Shader.h>

#define SKYBOX_SIZE 1000

extern Shader* skyboxShader;

extern void createSkybox();
extern void drawSkybox();
extern void freeSkybox();

#endif
