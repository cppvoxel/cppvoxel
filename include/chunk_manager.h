#ifndef CHUNK_MANAGER_H_
#define CHUNK_MANAGER_H_

#include <map>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "common.h"
#include "gl/shader.h"
#include "chunk.h"

using chunk_map = std::map<vec3i, std::shared_ptr<Chunk>>;
using chunk_it = chunk_map::iterator;

namespace ChunkManager {

extern chunk_map chunks;
extern GL::Shader* shader;

void init();
void free();
std::shared_ptr<Chunk> get(vec3i pos);
void update(vec3i camPos);
void draw(glm::mat4 projection, glm::mat4 view);

}

#endif
