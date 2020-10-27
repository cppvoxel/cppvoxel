#include "particle_manager.h"

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

void main(){
  FragColor = vec4(0.4, 0.8, 1.0, 1.0);
})";

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
  particle.size = (rand() % 11) / 100.0f + 0.1f;
  particle.pos = glm::vec3((rand() % 1000) - 500, 250.0f - (rand() % 50), (rand() % 1000) - 500) + cameraPos;
  particle.life = 2.5f;
  particle.speed = {0.0f, -300.0f, 0.0f};
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
  for(unsigned int i = 0; i < 20; i++){
    int unusedParticle = findUnusedParticle();
    respawnParticle(particles[unusedParticle], cameraPos);
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
      model = glm::scale(model, {p.size, p.size * 12.0f, p.size});
      shader->setMat4("model", model);

      glDrawArrays(GL_TRIANGLES, 0, 36);
    }
  }
}
