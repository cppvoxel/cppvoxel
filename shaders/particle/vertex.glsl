#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in mat4 aTransform;

flat out vec4 vColor;

uniform mat4 projection;
uniform mat4 view;

void main() {
  vColor = aColor;
  gl_Position = projection * view * aTransform * vec4(aPosition, 1.0);
}
