#include "particle_manager.h"

#include <glfw/glfw3.h>

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
layout (location = 0) in vec3 vPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main(){
  gl_Position = projection * view * model * vec4(vPosition, 1.0);
})";

const static char* shaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main(){
  FragColor = vec4(color, 1.0);
})";

enum WeatherType{
  NONE,
  RAIN,
  SNOW
};

double timeToEndWeatherCycle;
WeatherType weather = NONE;

int lastUsedParticle = 0;
int findUnusedParticle(){
  for(int i = lastUsedParticle; i < (int)ParticleManager::particles.size(); i++){
    if(ParticleManager::particles[i].life < 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  for(int i = 0; i < lastUsedParticle; i++){
    if(ParticleManager::particles[i].life < 0.0f){
      lastUsedParticle = i;
      return i;
    }
  }

  ParticleManager::particles.push_back(ParticleManager::particle_t());
  lastUsedParticle = (int)ParticleManager::particles.size() - 1;

  return lastUsedParticle;
}

void respawnParticle(ParticleManager::particle_t& particle, glm::vec3 cameraPos){
  particle.pos = glm::vec3((rand() % 1000) - 500, 250.0f - (rand() % 50), (rand() % 1000) - 500) + cameraPos;
  if(weather == RAIN){
    float size = (rand() % 11) / 100.0f + 0.1f;
    particle.size = {size, size * 12.0f, size};
    particle.life = 2.5f;
    particle.speed = {0.0f, -300.0f, 0.0f};
  }else{
    float size = (rand() % 11) / 100.0f + 0.2f;
    particle.size = {size, size, size};
    particle.life = 30.0f;
    particle.speed = {0.0f, -30.0f, 0.0f};
  }
}

void setWeatherCycle(){
  if(weather != NONE){
    timeToEndWeatherCycle = glfwGetTime() + 15.0;
    weather = NONE;
    return;
  }

  ParticleManager::particles.clear();
  weather = (WeatherType)((rand() % 2) + 1);
  timeToEndWeatherCycle = glfwGetTime() + 30.0;

  glm::vec3 color = {1.0f, 1.0f, 1.0f};
  if(weather == RAIN){
    color = {0.2f, 0.3f, 1.0f};
  }

  ParticleManager::shader->use();
  ParticleManager::shader->setVec3("color", color);
}

namespace ParticleManager{
  std::vector<particle_t> particles;
  Shader* shader;

  unsigned int vao;
}

void ParticleManager::init(){
  // pre-allocate particles
  for(unsigned int i = 0; i < 10000; i++){
    particles.push_back(particle_t());
  }

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

  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);
}

void ParticleManager::free(){
  delete shader;
  particles.clear();
  particles.shrink_to_fit();
}

void ParticleManager::update(double delta, glm::vec3 cameraPos){
  if(glfwGetTime() > timeToEndWeatherCycle){
    setWeatherCycle();
  }

  if(weather != NONE){
    for(unsigned int i = 0; i < 20; i++){
      int unusedParticle = findUnusedParticle();
      respawnParticle(particles[unusedParticle], cameraPos);
    }
  }

  for(particle_t& p : particles){
    p.life -= (float)delta;
    if(p.life > 0.0f){
      p.pos += p.speed * (float)delta;
    }
  }
}

void ParticleManager::draw(glm::mat4 projection, glm::mat4 view){
  shader->use();
  shader->setMat4("projection", projection);
  shader->setMat4("view", view);

  glBindVertexArray(vao);
  for(particle_t& p : particles){
    if(p.life > 0.0f){
      glm::mat4 model = glm::translate(glm::mat4(1.0f), p.pos);
      model = glm::scale(model, p.size);
      shader->setMat4("model", model);

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
  }
}
