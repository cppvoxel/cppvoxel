#include "particle_manager.h"

#include <stdio.h>

#include <glfw/glfw3.h>

#include "common.h"
#include "gl/instance_buffer.h"

const static uint RAIN_COLOR = (uint)(40 | (60 << 8) | (255 << 16) | (255 << 24));
const static uint SNOW_COLOR = (uint)(255 | (255 << 8) | (255 << 16) | (255 << 24));

const static int8_t vertices[] = {
  -1,-1,-1,
  -1,-1, 1,
  -1, 1, 1,
  1, 1,-1,
  -1,-1,-1,
  -1, 1,-1,
  1,-1, 1,
  -1,-1,-1,
  1,-1,-1,
  1, 1,-1,
  1,-1,-1,
  -1,-1,-1,
  -1,-1,-1,
  -1, 1, 1,
  -1, 1,-1,
  1,-1, 1,
  -1,-1, 1,
  -1,-1,-1,
  -1, 1, 1,
  -1,-1, 1,
  1,-1, 1,
  1, 1, 1,
  1,-1,-1,
  1, 1,-1,
  1,-1,-1,
  1, 1, 1,
  1,-1, 1,
  1, 1, 1,
  1, 1,-1,
  -1, 1,-1,
  1, 1, 1,
  -1, 1,-1,
  -1, 1, 1,
  1, 1, 1,
  -1, 1, 1,
  1,-1, 1
};

const static char* shaderVertexSource = R"(#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec4 aColor;
layout (location = 2) in mat4 aTransform;

flat out vec4 vColor;

uniform mat4 projection;
uniform mat4 view;

void main(){
  vColor = aColor;
  gl_Position = projection * view * aTransform * vec4(aPosition, 1.0);
})";

const static char* shaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

flat in vec4 vColor;

void main(){
  FragColor = vColor;
})";

enum WeatherType{
  NONE,
  RAIN,
  SNOW
};

double timeToEndWeatherCycle;
WeatherType weather;

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
    particle.life = 10.0f;
    particle.speed = -25.0f;
    particle.color = SNOW_COLOR;
  }
}

inline void setWeatherCycle(){
  weather = (WeatherType)((rand() % 2) + 1);
  timeToEndWeatherCycle = glfwGetTime() + 10.0;
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

  uint vao;
  uint particlesToDraw;
}

void ParticleManager::init(){
  // pre-allocate particles
  particles.resize(16380);

  shader = new GL::Shader(shaderVertexSource, shaderFragmentSource);
  shader->use();
  shaderProjectionLocation = shader->getUniformLocation("projection");
  shaderViewLocation = shader->getUniformLocation("view");

  setWeatherCycle();

  glGenVertexArrays(1, &vao);

  uint vbo;
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // vertices
  glVertexAttribPointer(0, 3, GL_BYTE, GL_FALSE, 3 * sizeof(int8_t), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribDivisor(0, 0);

  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);

  colorInstanceBuffer = new GL::InstanceBuffer<uint>(vao, particles.size(), 1);
  matrixInstanceBuffer = new GL::InstanceBuffer<glm::mat4>(vao, particles.size(), 2);
}

void ParticleManager::free(){
  particles.clear();
  particles.shrink_to_fit();

  delete colorInstanceBuffer;
  delete matrixInstanceBuffer;
  delete shader;

  glDeleteVertexArrays(1, &vao);
}

void ParticleManager::update(double delta, glm::vec3 cameraPos){
  if(glfwGetTime() > timeToEndWeatherCycle){
    setWeatherCycle();
  }

  if(weather != NONE){
    uint8_t amount = weather == RAIN ? 25 : 5;
    for(uint i = 0; i < amount; i++){
      respawnParticle(particles[findUnusedParticle()], cameraPos);
    }
  }

  colorInstanceBuffer->expand(particles.size());
  matrixInstanceBuffer->expand(particles.size());

  // double start = glfwGetTime();
  colorInstanceBuffer->bind();
  uint* colorPtr = (uint*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  matrixInstanceBuffer->bind();
  glm::mat4* matrixPtr = (glm::mat4*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  // printf("map particle buffers: %.4fms with %u particles\n", (glfwGetTime() - start) * 1000.0, particlesToDraw);

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
  glUnmapBuffer(GL_ARRAY_BUFFER);
  matrixInstanceBuffer->bind();
  glUnmapBuffer(GL_ARRAY_BUFFER);
}

void ParticleManager::draw(glm::mat4 projection, glm::mat4 view){
  shader->use();
  shader->setMat4(shaderProjectionLocation, projection);
  shader->setMat4(shaderViewLocation, view);

  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 36, particlesToDraw);
}
