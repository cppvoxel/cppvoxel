#version 330 core

out vec4 FragColor;

in vec3 vPosition;
in vec3 vTexCoord;
in float vDiffuse;

uniform sampler2DArray texture_array;

uniform int fog_near;
uniform int fog_far;

void main() {
  vec4 color = vec4(texture(texture_array, vTexCoord).rgb * vDiffuse, 1.0);
  color *= 1.0 - smoothstep(fog_near, fog_far, length(vPosition));

  if(color.a < 0.1) {
    discard;
  }

  FragColor = color;
}
