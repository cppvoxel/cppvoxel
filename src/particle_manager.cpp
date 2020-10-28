#include "particle_manager.h"

#include <stdio.h>

#include <glfw/glfw3.h>

#include "gl/instance_buffer.h"

const static unsigned int RAIN_COLOR = (unsigned int)(40 | (60 << 8) | (255 << 16) | (255 << 24));
const static unsigned int SNOW_COLOR = (unsigned int)(255 | (255 << 8) | (255 << 16) | (255 << 24));

const static float vertices[] = {
  -1.0f,-1.0f,-1.0f,
  -1.0f,-1.0f, 1.0f,
  -1.0f, 1.0f, 1.0f,
  1.0f, 1.0f,-1.0f,
  -1.0f,-1.0f,-1.0f,
  -1.0f, 1.0f,-1.0f,
  1.0f,-1.0f, 1.0f,
  -1.0f,-1.0f,-1.0f,
  1.0f,-1.0f,-1.0f,
  1.0f, 1.0f,-1.0f,
  1.0f,-1.0f,-1.0f,
  -1.0f,-1.0f,-1.0f,
  -1.0f,-1.0f,-1.0f,
  -1.0f, 1.0f, 1.0f,
  -1.0f, 1.0f,-1.0f,
  1.0f,-1.0f, 1.0f,
  -1.0f,-1.0f, 1.0f,
  -1.0f,-1.0f,-1.0f,
  -1.0f, 1.0f, 1.0f,
  -1.0f,-1.0f, 1.0f,
  1.0f,-1.0f, 1.0f,
  1.0f, 1.0f, 1.0f,
  1.0f,-1.0f,-1.0f,
  1.0f, 1.0f,-1.0f,
  1.0f,-1.0f,-1.0f,
  1.0f, 1.0f, 1.0f,
  1.0f,-1.0f, 1.0f,
  1.0f, 1.0f, 1.0f,
  1.0f, 1.0f,-1.0f,
  -1.0f, 1.0f,-1.0f,
  1.0f, 1.0f, 1.0f,
  -1.0f, 1.0f,-1.0f,
  -1.0f, 1.0f, 1.0f,
  1.0f, 1.0f, 1.0f,
  -1.0f, 1.0f, 1.0f,
  1.0f,-1.0f, 1.0f
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

unsigned int lastUsedParticle = 0;
unsigned int findUnusedParticle(){
  for(unsigned int i = lastUsedParticle; i < (unsigned int)ParticleManager::particles.size(); i++){
    if(ParticleManager::particles[i].life <= 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  for(unsigned int i = 0; i < lastUsedParticle; i++){
    if(ParticleManager::particles[i].life <= 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  printf("resizing particles (%u;%u)\n", (unsigned int)ParticleManager::particles.size(), lastUsedParticle);
  lastUsedParticle = (unsigned int)ParticleManager::particles.size();
  ParticleManager::particles.resize(ParticleManager::particles.size() * 2);

  return lastUsedParticle;
}

void respawnParticle(ParticleManager::particle_t& particle, glm::vec3 cameraPos){
  particle.pos = glm::vec3(
    (rand() % 1000) - 500,   // x
    300.0f - (rand() % 100), // y
    (rand() % 1000) - 500    // z
  ) + cameraPos;

  if(weather == RAIN){
    float size = (rand() % 11) / 100.0f + 0.1f;
    particle.size = {size, size * 20.0f, size};
    particle.life = 3.5f;
    particle.speed = -300.0f;
    particle.color = RAIN_COLOR;
  }else{
    float size = (rand() % 11) / 100.0f + 0.2f;
    particle.size = {size, size, size};
    particle.life = 17.0f;
    particle.speed = -35.0f;
    particle.color = SNOW_COLOR;
  }
}

inline void setWeatherCycle(){
  weather = (WeatherType)((rand() % 2) + 1);
  timeToEndWeatherCycle = glfwGetTime() + 10.0;
  printf("weather changed to %d\n", weather);
}

namespace ParticleManager{
  std::vector<particle_t> particles;
  Shader* shader;
  GL::InstanceBuffer<unsigned int>* colorInstanceBuffer;
  GL::InstanceBuffer<glm::mat4>* matrixInstanceBuffer;

  unsigned int vao;
  unsigned int particlesToDraw;
}

void ParticleManager::init(){
  // pre-allocate particles
  particles.resize(20480);

  shader = new Shader(shaderVertexSource, shaderFragmentSource);
  setWeatherCycle();

  glGenVertexArrays(1, &vao);

  unsigned int vbo;
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // vertices
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribDivisor(0, 0);

  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);

  colorInstanceBuffer = new GL::InstanceBuffer<unsigned int>(vao, 1024, 1);
  matrixInstanceBuffer = new GL::InstanceBuffer<glm::mat4>(vao, 1024, 2);
}

void ParticleManager::free(){
  particles.clear();
  particles.shrink_to_fit();

  delete colorInstanceBuffer;
  delete matrixInstanceBuffer;
  delete shader;
}

void ParticleManager::update(double delta, glm::vec3 cameraPos){
  if(glfwGetTime() > timeToEndWeatherCycle){
    setWeatherCycle();
  }

  if(weather != NONE){
    for(unsigned int i = 0; i < 20; i++){
      unsigned int unusedParticle = findUnusedParticle();
      respawnParticle(particles[unusedParticle], cameraPos);
    }
  }

  if(particles.size() == 0){
    return;
  }

  colorInstanceBuffer->expand(particles.size());
  matrixInstanceBuffer->expand(particles.size());

  colorInstanceBuffer->bind();
  unsigned int* colorPtr = (unsigned int*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
  matrixInstanceBuffer->bind();
  glm::mat4* matrixPtr = (glm::mat4*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);

  unsigned int bufferIndex = 0;
  for(particle_t& p : particles){
    p.life -= (float)delta;
    if(p.life > 0.0f){
      p.pos += glm::vec3(0.0f, p.speed * (float)delta, 0.0f);

      glm::mat4 transform = glm::translate(glm::mat4(1.0f), p.pos);
      transform = glm::scale(transform, p.size);

      // write directly to memory 😬
      colorPtr[bufferIndex] = p.color;
      matrixPtr[bufferIndex++] = transform;
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
  shader->setMat4("projection", projection);
  shader->setMat4("view", view);

  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLES, 0, 36, particlesToDraw);
}
