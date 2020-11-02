#include "gl/shader.h"

#include <stdio.h>

namespace GL{

Shader::Shader(ShaderSource source){
  int success;

  uint vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &source.vertex, NULL);
  glCompileShader(vertexShader);

  // check compile errors
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success){
    char infoLog[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    fprintf(stderr, "vertex shader error\n");
    fprintf(stderr, infoLog);
    fprintf(stderr, "\n");

    glDeleteShader(vertexShader);
    return;
  }

  uint fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &source.fragment, NULL);
  glCompileShader(fragmentShader);

  // check compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success){
    char infoLog[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    fprintf(stderr, "fragment shader error\n");
    fprintf(stderr, infoLog);
    fprintf(stderr, "\n");

    glDeleteShader(fragmentShader);
    return;
  }

  uint shaderProgram;
  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // check compile errors
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if(!success){
    char infoLog[512];
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    printf("shader program error\n");
    printf(infoLog);
    printf("\n");

    glDeleteShader(shaderProgram);
    return;
  }

  id = shaderProgram;
}

Shader::~Shader(){
  glDeleteProgram(id);
  id = 0;
}

void Shader::use(){
  if(id == 0){
    fprintf(stderr, "ERROR: shader id = 0\n");
    exit(-1);
  }

  glUseProgram(id);
}

}
