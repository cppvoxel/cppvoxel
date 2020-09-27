#ifndef CHUNK_MANAGER_H_
#define CHUNK_MANAGER_H_

#include <map>

#include <Shader.h>

#include "common.h"
#include "chunk.h"

typedef std::map<vec3i, Chunk*>::iterator chunk_it;

namespace ChunkManager{

extern std::map<vec3i, Chunk*> chunks;
extern Shader* shader;

void init();
void free();
Chunk* get(vec3i pos);
void update(vec3i camPos, int distance);

}

#endif
