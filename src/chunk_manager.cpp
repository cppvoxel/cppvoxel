#include "chunk_manager.h"

const static char* shaderVertexSource = R"(#version 330 core
layout (location = 0) in int aVertex;

out vec3 vPosition;
out vec3 vTexCoord;
out float vDiffuse;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

const vec3 sun_direction = normalize(vec3(1, 3, 2));
const float ambient = 0.4f;

void main(){
  vec3 aPosition = vec3(float(aVertex & (63)), float((aVertex >> 6) & (63)), float((aVertex >> 12) & (63)));
  int aNormal = (aVertex >> 18) & (7);
  vec3 normal;
  // if(aNormal == 0){
  //   aPosition.y += 1.0;
  // }else if(aNormal == 2){
  //   aPosition.x += 1.0;
  // }else if(aNormal == 4){
  //   aPosition.z += 1.0;
  // }
  if(aNormal == 0){
    normal = vec3(0.0, 1.0, 0.0);
  }else if(aNormal == 1){
    normal = vec3(0.0, -1.0, 0.0);
  }else if(aNormal == 2){
    normal = vec3(1.0, 0.0, 0.0);
  }else if(aNormal == 3){
    normal = vec3(-1.0, 0.0, 0.0);
  }else if(aNormal == 4){
    normal = vec3(0.0, 0.0, 1.0);
  }else if(aNormal == 5){
    normal = vec3(0.0, 0.0, -1.0);
  }

  int aTextureId = (aVertex >> 21) & (255);

  vPosition = (view * model * vec4(aPosition, 1.0)).xyz;
  vTexCoord = vec3(float((aVertex >> 29) & (1)), float((aVertex >> 30) & (1)), aTextureId);
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
  vec4 color = vec4(texture(texture_array, vTexCoord).rgb * vDiffuse, 1.0);
  color *= 1.0 - smoothstep(fog_near, fog_far, length(vPosition));
  color = vec4(pow(color.rgb, vec3(1.0 / 2.2)), color.a);

  if(color.a < 0.1){
    discard;
  }

  FragColor = color;
})";

namespace ChunkManager{
  chunk_map chunks;
  Shader* shader;
}

void ChunkManager::init(){
  shader = new Shader(shaderVertexSource, shaderFragmentSource);
  shader->use();
  shader->setInt("texture_array", 0);
  shader->setInt("fog_near", (viewDistance + 1) * CHUNK_SIZE - 8);
  shader->setInt("fog_far", (viewDistance + 1) * CHUNK_SIZE - 8);
}

void ChunkManager::free(){
  // for(chunk_it it = chunks.begin(); it != chunks.end(); it++){
  //   std::shared_ptr<Chunk> chunk = it->second;
  //   delete chunk;
  // }

  delete shader;
}

std::shared_ptr<Chunk> ChunkManager::get(vec3i pos){
  chunk_it it = chunks.find(pos);
  if(it != chunks.end()){
    return it->second;
  }

  return nullptr;
}

void ChunkManager::update(vec3i camPos, int distance){
  vec3i chunkPos;

  for(int i = -distance; i <= distance; i++){
    for(int j = -distance; j <= distance; j++){
      for(int k = -distance; k <= distance; k++){
        chunkPos.x = camPos.x + i;
        chunkPos.y = camPos.y + k;
        chunkPos.z = camPos.z + j;

        std::shared_ptr<Chunk> chunk = get(chunkPos);
        if(!chunk){
          // chunks[chunkPos] = std::make_shared<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z);
          if(!chunks.insert(std::make_pair(chunkPos, std::make_shared<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z))).second){
            printf("something idk %d %d %d\n", chunkPos.x, chunkPos.y, chunkPos.z);
          }
          // continue;
        }
      }
    }
  }
}
