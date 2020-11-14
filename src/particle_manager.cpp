#include "particle_manager.h"

#include <stdio.h>

#include "common.h"
#include "glfw/glfw.h"
#include "gl/utils.h"
#include "gl/instance_buffer.h"
#include "gl/vao.h"
#include "gl/buffer.h"

const static uint RAIN_COLOR = (uint)(40 | (60 << 8) | (255 << 16) | (255 << 24));
const static uint SNOW_COLOR = (uint)(255 | (255 << 8) | (255 << 16) | (255 << 24));

enum WeatherType{
  NONE,
  RAIN,
  SNOW
};

double timeToEndWeatherCycle;
WeatherType weather;
double timeToSpawnParticles;
const double PARTICLE_SPAWN_INTERVAL = 0.05; // seconds

uint lastUsedParticle = 0;
uint findUnusedParticle(){
  for(uint i = lastUsedParticle; i < (uint)ParticleManager::particles.size(); i++){
    if(ParticleManager::particles[i].life <= 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  for(uint i = 0; i < lastUsedParticle; i++){
    if(ParticleManager::particles[i].life <= 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  printf("resizing particles (%u;%u)\n", (uint)ParticleManager::particles.size(), lastUsedParticle);
  lastUsedParticle = (uint)ParticleManager::particles.size();
  ParticleManager::particles.resize(ParticleManager::particles.size() + 2048);

  return lastUsedParticle;
}

void respawnParticle(ParticleManager::particle_t& particle, glm::vec3 cameraPos){
  particle.pos = glm::vec3(
    (rand() % 1000) - 500,   // x
    250.0f - (rand() % 100), // y
    (rand() % 1000) - 500    // z
  ) + cameraPos;

  if(weather == RAIN){
    float size = (rand() % 11) / 100.0f + 0.1f;
    particle.size = {size, size * 20.0f, size};
    particle.life = 2.0f;
    particle.speed = -300.0f;
    particle.color = RAIN_COLOR;
  }else{
    float size = (rand() % 11) / 100.0f + 0.2f;
    particle.size = {size, size, size};
    particle.life = 15.0f;
    particle.speed = -25.0f;
    particle.color = SNOW_COLOR;
  }
}

inline void setWeatherCycle(){
  weather = (WeatherType)((rand() % 2) + 1);
  timeToEndWeatherCycle = GLFW::getTime() + 10.0;
  printf("weather changed to %d\n", weather);
}

inline glm::mat4 particleMatrix(glm::vec3 scale, glm::vec3 position){
  return {
    scale.x, 0.0f, 0.0f, 0.0f,
    0.0f, scale.y, 0.0f, 0.0f,
    0.0f, 0.0f, scale.z, 0.0f,
    position.x, position.y, position.z, 1.0f
  };
}

namespace ParticleManager{
  std::vector<particle_t> particles;

  GL::Shader* shader;
  int shaderProjectionLocation, shaderViewLocation;

  GL::InstanceBuffer<uint>* colorInstanceBuffer;
  GL::InstanceBuffer<glm::mat4>* matrixInstanceBuffer;

  GL::VAO* vao;
  uint particlesToDraw;
}

void ParticleManager::init(){
  // pre-allocate particles
  particles.resize(16380);

  shader = new GL::Shader(GL::Shaders::particle);
  shader->use();
  shaderProjectionLocation = shader->getUniformLocation("projection");
  shaderViewLocation = shader->getUniformLocation("view");

  setWeatherCycle();

  vao = new GL::VAO();
  GL::Buffer<GL::ARRAY>* vbo = new GL::Buffer<GL::ARRAY>();

  vao->bind();
  vbo->data(sizeof(cube_vertices), cube_vertices);
  vao->attrib<int8_t>(0, 4, GL::BYTE, 0);

  GL::VAO::unbind();
  delete vbo;

  colorInstanceBuffer = new GL::InstanceBuffer<uint>(vao, particles.size(), 1);
  matrixInstanceBuffer = new GL::InstanceBuffer<glm::mat4>(vao, particles.size(), 2);

  timeToSpawnParticles = GLFW::getTime();
}

void ParticleManager::free(){
  particles.clear();
  particles.shrink_to_fit();

  delete vao;
  delete colorInstanceBuffer;
  delete matrixInstanceBuffer;
  delete shader;
}

void ParticleManager::update(double delta, glm::vec3 cameraPos){
  if(GLFW::getTime() > timeToEndWeatherCycle){
    setWeatherCycle();
  }

  if(weather != NONE && timeToSpawnParticles <= GLFW::getTime()){
    timeToSpawnParticles += PARTICLE_SPAWN_INTERVAL;
    uint8_t amount = weather == RAIN ? 100 : 15;
    for(uint i = 0; i < amount; i++){
      respawnParticle(particles[findUnusedParticle()], cameraPos);
    }
  }

  colorInstanceBuffer->expand(particles.size());
  matrixInstanceBuffer->expand(particles.size());

  // double start = GLFW::getTime();
  colorInstanceBuffer->bind();
  uint* colorPtr = GL::mapBuffer<uint>();
  matrixInstanceBuffer->bind();
  glm::mat4* matrixPtr = GL::mapBuffer<glm::mat4>();
  // printf("map particle buffers: %.4fms with %u particles\n", (GLFW::getTime() - start) * 1000.0, particlesToDraw);

  uint bufferIndex = 0;
  for(particle_t& p : particles){
    p.life -= (float)delta;
    if(p.life > 0.0f){
      p.pos += glm::vec3(0.0f, p.speed * (float)delta, 0.0f);

      // write directly to memory 😬
      colorPtr[bufferIndex] = p.color;
      matrixPtr[bufferIndex++] = particleMatrix(p.size, p.pos);
    }
  }
  particlesToDraw = bufferIndex;

  colorInstanceBuffer->bind();
  GL::unmapBuffer();
  matrixInstanceBuffer->bind();
  GL::unmapBuffer();
}

void ParticleManager::draw(glm::mat4 projection, glm::mat4 view){
  shader->use();
  shader->setMat4(shaderProjectionLocation, projection);
  shader->setMat4(shaderViewLocation, view);

  vao->bind();
  GL::drawInstanced(48, particlesToDraw);
}
