#include "shader.h"

#include <stdio.h>

Shader::Shader(const char* vertexSource, const char* fragmentSource){
  int success;

  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  // check compile errors
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success){
    char infoLog[512];
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    printf("vertex shader error\n");
    printf(infoLog);
    printf("\n");

    glDeleteShader(vertexShader);
    return;
  }

  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // check compile errors
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success){
    char infoLog[512];
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    printf("fragment shader error\n");
    printf(infoLog);
    printf("\n");

    glDeleteShader(fragmentShader);
    return;
  }

  unsigned int shaderProgram;
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
    printf("WARNING: shader id = 0\n");
  }

  glUseProgram(id);
}
