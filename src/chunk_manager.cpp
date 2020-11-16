#include "chunk_manager.h"

#include "timer.h"

/**
  * @brief Checks if the given chunk matrix is visible
  * @return bool Chunk visible
*/
inline bool isChunkInsideFrustum(glm::mat4 mvp){
  glm::vec4 center = mvp * glm::vec4(CHUNK_SIZE / 2, CHUNK_SIZE / 2, CHUNK_SIZE / 2, 1);
  center.x /= center.w;
  center.y /= center.w;

  return !(center.z < -CHUNK_SIZE / 2 || fabsf(center.x) > 1 + fabsf(CHUNK_SIZE * 2 / center.w) || fabsf(center.y) > 1 + fabsf(CHUNK_SIZE * 2 / center.w));
}

namespace ChunkManager{
  chunk_map chunks;
  vec3i cameraPos;

  GL::Shader* shader;
  int shaderProjectionLocation, shaderViewLocation, shaderModelLocation;
}

void ChunkManager::init(){
  shader = new GL::Shader(GL::Shaders::chunk);
  shader->use();

  shader->setInt("texture_array", 0);
  shader->setInt("fog_near", (viewDistance + 1) * CHUNK_SIZE - 4);
  shader->setInt("fog_far", (viewDistance + 1) * CHUNK_SIZE);

  shaderProjectionLocation = shader->getUniformLocation("projection");
  shaderViewLocation = shader->getUniformLocation("view");
  shaderModelLocation = shader->getUniformLocation("model");
}

void ChunkManager::free(){
  delete shader;
}

std::shared_ptr<Chunk> ChunkManager::get(vec3i pos){
  chunk_it it = chunks.find(pos);
  if(it != chunks.end()){
    return it->second;
  }

  return nullptr;
}

void ChunkManager::update(vec3i camPos){
  cameraPos = camPos;
  vec3i chunkPos;
  const int distance = viewDistance + 1;

  for(int i = -distance; i <= distance; i++){
    for(int j = -distance; j <= distance; j++){
      for(int k = -distance; k <= distance; k++){
        chunkPos.x = cameraPos.x + i;
        chunkPos.y = cameraPos.y + k;
        chunkPos.z = cameraPos.z + j;

        if(!get(chunkPos)){
          chunks.insert(std::make_pair(chunkPos, std::make_shared<Chunk>(chunkPos.x, chunkPos.y, chunkPos.z)));
        }
      }
    }
  }
}

void ChunkManager::draw(glm::mat4 projection, glm::mat4 view){
  shader->use();
  shader->setMat4(shaderProjectionLocation, projection);
  shader->setMat4(shaderViewLocation, view);

  uint chunksDeleted = 0;
  uint chunksGenerated = 0;
  int dx, dy, dz;

  glm::mat4 pv = projection * view;

#ifdef PRINT_TIMING
  Timer timer;
#endif
  for(chunk_it it = ChunkManager::chunks.begin(); it != ChunkManager::chunks.end(); it++){
    std::shared_ptr<Chunk> chunk = it->second;

    dx = cameraPos.x - chunk->x;
    dy = cameraPos.y - chunk->y;
    dz = cameraPos.z - chunk->z;

    // don't render chunks outside of generation radius
    if(abs(dx) > viewDistance + 1 || abs(dy) > viewDistance + 1 || abs(dz) > viewDistance + 1){
      if(chunksDeleted < (uint)maxChunksDeletedPerFrame){
        STACK_TRACE_PUSH("remove chunk")
        it = ChunkManager::chunks.erase(it);
        chunk.reset();
        chunksDeleted++;
      }

      continue;
    }

    // don't render invisible chunks
    if(chunk->empty || abs(dx) > viewDistance || abs(dy) > viewDistance || abs(dz) > viewDistance || !isChunkInsideFrustum(pv * chunk->model)){
      continue;
    }

    // update chunk if needed
    if(chunk->changed && chunksGenerated < (uint)maxChunksGeneratedPerFrame){
      if(chunk->update() && chunk->elements > 0){
        chunksGenerated++;
      }
    }

    // don't draw if chunk has no mesh
    if(chunk->elements == 0){
      continue;
    }

    ChunkManager::shader->setMat4(shaderModelLocation, chunk->model);
    chunk->draw();
  }

#ifdef PRINT_TIMING
  printf("draw all chunks: ");
#endif
}
