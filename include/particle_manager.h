#ifndef PARTICLE_MANAGER_H_
#define PARTICLE_MANAGER_H_

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.h"

namespace ParticleManager{

struct particle_t{
	glm::vec3 pos, size;
  unsigned int color;
	float speed, life;

  particle_t() : pos({0.0f, 0.0f, 0.0f}), size({1.0f, 1.0f, 1.0f}), speed(0.0f), life(0.0f){}
};

extern std::vector<particle_t> particles;
extern Shader* shader;

void init();
void free();
void update(double delta, glm::vec3 cameraPos);
void draw(glm::mat4 projection, glm::mat4 view);

}

#endif
