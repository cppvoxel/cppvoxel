#include "chunk_manager.h"

const static char* shaderVertexSource = R"(#version 330 core
layout (location = 0) in vec4 coord;
// layout (location = 1) in int brightness;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texCoord;

out vec3 vPosition;
out vec3 vTexCoord;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const vec3 sun_direction = normalize(vec3(1, 3, 2));
const float ambient = 0.4f;

void main(){
  vPosition = (view * model * vec4(coord.xyz, 1.0)).xyz;
  vTexCoord = vec3(texCoord.xy, coord.w);
  vDiffuse = (max(dot(normal, sun_direction), 0.0) + ambient) /** (brightness / 5.0)*/;

  gl_Position = projection * vec4(vPosition, 1.0);
})";

const static char* shaderFragmentSource = R"(#version 330 core
out vec4 FragColor;

in vec3 vPosition;
in vec3 vTexCoord;
in float vDiffuse;

uniform sampler2DArray texture_array;

uniform int fog_near;
uniform int fog_far;

void main(){
  FragColor = vec4(texture(texture_array, vTexCoord).rgb * vDiffuse, 1.0);
  FragColor *= 1.0 - smoothstep(fog_near, fog_far, length(vPosition));
  FragColor = vec4(pow(FragColor.rgb, vec3(1.0 / 2.2)), FragColor.a);
})";

namespace ChunkManager{
  std::map<vec3i, Chunk*> chunks;
  Shader* shader;
}

void ChunkManager::init(){
  shader = new Shader(shaderVertexSource, shaderFragmentSource);
}

void ChunkManager::free(){
  for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
    Chunk* chunk = it->second;
    delete chunk;
  }

  delete shader;
}

Chunk* ChunkManager::get(vec3i pos){
  chunk_it it = chunks.find(pos);
  if(it != chunks.end()){
    return it->second;
  }

  return NULL;
}

void ChunkManager::update(vec3i camPos, int distance){
  vec3i chunkPos;
  Chunk* chunk;

  for(int i = -distance; i <= distance; i++){
    for(int j = -distance; j <= distance; j++){
      for(int k = -distance; k <= distance; k++){
        chunkPos.x = camPos.x + i;
        chunkPos.y = camPos.y + k;
        chunkPos.z = camPos.z + j;

        if(get(chunkPos) != NULL){
          continue;
        }

        chunk = new Chunk(chunkPos.x, chunkPos.y, chunkPos.z);
        STACK_TRACE_PUSH("add chunk")
        chunks[chunkPos] = chunk;
      }
    }
  }
}
