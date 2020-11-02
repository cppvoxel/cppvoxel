#version 330 core

layout (location = 0) in int aVertex;

out vec3 vPosition;
out vec3 vTexCoord;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const vec3 sun_direction = normalize(vec3(1, 3, 2));
const float ambient = 0.4f;

const vec3 normalCoords[] = vec3[](
  vec3(0.0, 1.0, 0.0),
  vec3(0.0, -1.0, 0.0),
  vec3(1.0, 0.0, 0.0),
  vec3(-1.0, 0.0, 0.0),
  vec3(0.0, 0.0, 1.0),
  vec3(0.0, 0.0, -1.0)
);

void main(){
  vec3 aPosition = vec3(float(aVertex & (63)), float((aVertex >> 6) & (63)), float((aVertex >> 12) & (63)));
  int aNormal = (aVertex >> 18) & (7);

  int aTextureId = (aVertex >> 21) & (255);

  vPosition = (view * model * vec4(aPosition, 1.0)).xyz;
  vTexCoord = vec3(float((aVertex >> 29) & (1)), float((aVertex >> 30) & (1)), aTextureId);
  vDiffuse = (max(dot(normalCoords[aNormal], sun_direction), 0.0) + ambient);

  gl_Position = projection * vec4(vPosition, 1.0);
}
