#include "common.h"

std::map<vec3i, Chunk*> chunks;

Chunk* getChunk(vec3i pos){
  chunk_it it = chunks.find(pos);
  if(it != chunks.end()){
    return it->second;
  }

  return NULL;
}
