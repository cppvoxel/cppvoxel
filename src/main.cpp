#include <stdio.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <Window.h>
#include <Shader.h>
#include <Texture.h>
#include <Camera.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

const static char* basicShaderVertexSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 MVP;

void main(){
  gl_Position = MVP * vec4(aPos, 1.0);
  TexCoord = vec2(aTexCoord.x, aTexCoord.y);
})";

const static char* basicShaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D tex;

void main(){
  FragColor = texture(tex, TexCoord);
})";

void processInput(GLFWwindow *window){
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    glfwSetWindowShouldClose(window, true);
  }

  if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
    camera.processKeyboard(FORWARD, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
    camera.processKeyboard(BACKWARD, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
    camera.processKeyboard(LEFT, deltaTime);
  }
  if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
    camera.processKeyboard(RIGHT, deltaTime);
  }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos){
  if(firstMouse){
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos;

  lastX = xpos;
  lastY = ypos;

  camera.processMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset){
  camera.processMouseScroll(yoffset);
}

int main(int argc, char** argv){
  Window window(WINDOW_WIDTH, WINDOW_HEIGHT, "cppgl example");

  glfwSetCursorPosCallback(window.window, mouseCallback);
  glfwSetScrollCallback(window.window, scrollCallback);
  glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glewExperimental = true;
  if(glewInit() != GLEW_OK){
    fprintf(stderr, "Failed to initialize GLEW\n");
    return -1;
  }

  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
  };

  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);


  Texture teuxtre("wall.png");

  Shader shader(basicShaderVertexSource, basicShaderFragmentSource);
  shader.use();
  shader.setInt("tex", 0);

  glm::mat4 model = glm::mat4(1.0f);

  model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(1.0f, 1.0f, 0.0f));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

  while(!window.shouldClose()){
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window.window);

    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(camera.fov), (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 mvp = projection * camera.getViewMatrix() * model;
    shader.setMat4("MVP", mvp);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    window.pollEvents();
    window.swapBuffers();
  }

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);

  return 0;
}
