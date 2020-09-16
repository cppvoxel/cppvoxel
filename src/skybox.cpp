#include "skybox.h"

#include <GL/glew.h>

const static char* skyboxShaderVertexSource = R"(#version 330 core
layout (location = 0) in vec3 position;

out vec3 vCoord;

uniform mat4 projection;
uniform mat4 view;

void main(){
  vCoord = position;

  gl_Position = (projection * mat4(mat3(view))) * vec4(position, 1.0);
})";

const static char* skyboxShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec3 vCoord;

void main(){
  vec3 sky_normal = normalize(vCoord.xyz);
  float gradient = dot(sky_normal, vec3(0.0, 1.0, 0.0));
  FragColor = vec4(vec3(0.3f, 0.6f, 0.8f) * vec3(gradient + 0.5), 1.0);
})";

const short skyboxVertices[] = {
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE, -SKYBOX_SIZE,
  -SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
   SKYBOX_SIZE,  SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE, -SKYBOX_SIZE,
   SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE,
  -SKYBOX_SIZE, -SKYBOX_SIZE,  SKYBOX_SIZE
};

const unsigned char skyboxIndices[] = {
  2, 1, 0, 0, 3, 2,
  6, 5, 4, 4, 7, 6,
  10, 9, 8, 8, 11, 10,
  14, 13, 12, 12, 15, 14,
  18, 17, 16, 16, 19, 18,
  22, 21, 20, 20, 23, 22
};

unsigned int skyboxVao;
Shader* skyboxShader;

inline void createSkybox(){
  skyboxShader = new Shader(skyboxShaderVertexSource, skyboxShaderFragmentSource);

  unsigned int vbo, ebo;
  glGenVertexArrays(1, &skyboxVao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(skyboxVao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

  // position
  glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, 3 * sizeof(short), (void*)0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
}

inline void drawSkybox(){
  glBindVertexArray(skyboxVao);
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
}

inline void freeSkybox(){
  glDeleteVertexArrays(1, &skyboxVao);
}
