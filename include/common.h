#ifndef COMMON_H_
#define COMMON_H_

#include <map>

struct vec3i{
  int x;
  int y;
  int z;
};

class Chunk;
std::map<vec3i, Chunk*> chunks;
typedef std::map<vec3i, Chunk*>::iterator chunk_it;

Chunk* getChunk(vec3i pos){
  chunk_it it = chunks.find(pos);
  if(it != chunks.end()){
    return it->second;
  }

  return NULL;
}

#endif
