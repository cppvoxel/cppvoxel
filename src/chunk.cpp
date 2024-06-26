#include "chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <glm/gtc/noise.hpp>

#include "gl/buffer.h"

#include "chunk_manager.h"
#include "timer.h"

#define sign(_x) ({ __typeof__(_x) _xx = (_x);\
  ((__typeof__(_x)) ( (((__typeof__(_x)) 0) < _xx) - (_xx < ((__typeof__(_x)) 0))));})

#define WATER_LEVEL 57

enum NormalFace : uint8_t {
  PY = 0,
  NY,
  PX,
  NX,
  PZ,
  NZ
};

inline float lerp(float a, float b, float t) {
  return a * (1.0f - t) + b * t;
}

inline ushort blockIndex(uint8_t x, uint8_t y, uint8_t z) {
  return x | (y << 5) | (z << 10);
}

// check if a block ID is transparent
inline bool isTransparent(block_t block) {
  return block == AIR || block == GLASS;
}

/*
  x y z 6 bits
  normal 3 bits
  textureId 8 bits
  texX texY 1 bit
*/
inline int packVertex(uint8_t x, uint8_t y, uint8_t z, NormalFace normal, uint8_t textureId, uint8_t texX, uint8_t texY) {
  return x | (y << 6) | (z << 12) | (normal << 18) | (textureId << 21) | (texX << 29) | (texY << 30);
}

float getHeight(int x, int z, int xCS, int zCS, int octaves, float roughness, float smoothness, float amplitude) {
  float xCoord = (float)(x + xCS);
  float zCoord = (float)(z + zCS);
  float totalValue = 0.0f;

  float frequency, _amplitude;

  for(int octave = 0; octave < octaves - 1; octave++) {
    frequency = glm::pow(2.0f, octave);
    _amplitude = glm::pow(roughness, octave);
    totalValue += glm::simplex(glm::vec2{xCoord* frequency / smoothness, zCoord* frequency / smoothness}) * _amplitude;
  }

  return ((totalValue / 2.1f) + 1.2f) * amplitude;
}

Chunk::Chunk(int _x, int _y, int _z) {
#ifdef PRINT_TIMING
  Timer timer;
  unsigned short count = 0;
#endif

  blocks = (block_t*)malloc(CHUNK_SIZE_CUBED * sizeof(block_t));

  vao = nullptr;
  elements = 0;
  changed = false;
  empty = true;
  meshChanged = false;

  x = _x;
  y = _y;
  z = _z;

  // precalculate chunk position in block coordinates
  const int xCS = x * CHUNK_SIZE;
  const int yCS = y * CHUNK_SIZE;
  const int zCS = z * CHUNK_SIZE;

  model = glm::translate(glm::mat4(1.0f), glm::vec3(xCS, yCS, zCS));

  int height, thickness;
  uint8_t dx, dy, dz;
  block_t block;

  for(dx = 0; dx < CHUNK_SIZE; dx++) {
    for(dz = 0; dz < CHUNK_SIZE; dz++) {
      height = (int)getHeight(dx, dz, xCS, zCS, 8, 0.4f, 400.0f, 100.0f);

      for(dy = 0; dy < CHUNK_SIZE; dy++) {
        if(height < WATER_LEVEL) {
          height = WATER_LEVEL;
        }

        thickness = height - (dy + yCS);

        block = dy + yCS <= height ?
                height <= WATER_LEVEL && thickness <= 1 ? WATER :
                height <= WATER_LEVEL + 3 && thickness <= 4 ? SAND :
                thickness == 0 ? GRASS :
                thickness <= 3 ? DIRT :
                STONE
                : AIR;
        blocks[blockIndex(dx, dy, dz)] = block;

        if(empty && block != AIR) {
          changed = true;
          empty = false;
        }

#ifdef PRINT_TIMING

        if(block != AIR) {
          count++;
        }

#endif
      }
    }
  }

#ifdef PRINT_TIMING
  printf("chunk gen: %d blocks ", count);
#endif
}

Chunk::~Chunk() {
  if(vao != nullptr) {
    delete vao;
  }

  // delete the stored data
  free(blocks);
}

// update the chunk
bool Chunk::update() {
  // if the chunk does not need to remesh then stop
  if(!changed) {
    return false;
  }

  STACK_TRACE_PUSH("chunk neighbors")

  // get chunk neighbors
  std::shared_ptr<Chunk> px = ChunkManager::get({x + 1, y, z});
  std::shared_ptr<Chunk> nx = ChunkManager::get({x - 1, y, z});
  std::shared_ptr<Chunk> py = ChunkManager::get({x, y + 1, z});
  std::shared_ptr<Chunk> ny = ChunkManager::get({x, y - 1, z});
  std::shared_ptr<Chunk> pz = ChunkManager::get({x, y, z + 1});
  std::shared_ptr<Chunk> nz = ChunkManager::get({x, y, z - 1});

  if(!px || !nx || !py || !ny || !pz || !nz) {
    return false;
  }

  STACK_TRACE_PUSH("update chunk")

#ifdef PRINT_TIMING
  Timer timer;
#endif

  // updating is taken care of - reset flag
  changed = false;

  uint8_t w, _x, _y, _z;
  block_t block;

  for(_z = 0; _z < CHUNK_SIZE; _z++) {
    for(_x = 0; _x < CHUNK_SIZE; _x++) {
      for(_y = 0; _y < CHUNK_SIZE; _y++) {
        block = blocks[blockIndex(_x, _y, _z)];

        if(block == AIR) {
          continue;
        }

        // add a face if -x is transparent
        if(isTransparent(get(_x - 1, _y, _z, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][0]; // get texture coordinates

          vertexData.push_back(packVertex(_x, _y, _z, NX, w, 0, 0));
          vertexData.push_back(packVertex(_x, _y + 1, _z + 1, NX, w, 1, 1));
          vertexData.push_back(packVertex(_x, _y + 1, _z, NX, w, 0, 1));
          vertexData.push_back(packVertex(_x, _y, _z, NX, w, 0, 0));
          vertexData.push_back(packVertex(_x, _y, _z + 1, NX, w, 1, 0));
          vertexData.push_back(packVertex(_x, _y + 1, _z + 1, NX, w, 1, 1));
        }

        // add a face if +x is transparent
        if(isTransparent(get(_x + 1, _y, _z, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][1]; // get texture coordinates

          vertexData.push_back(packVertex(_x + 1, _y, _z, PX, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PX, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y, _z + 1, PX, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y, _z, PX, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z, PX, w, 1, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PX, w, 0, 1));
        }

        // add a face if -z is transparent
        if(isTransparent(get(_x, _y, _z - 1, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][4]; // get texture coordinates

          vertexData.push_back(packVertex(_x, _y, _z, NZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z, NZ, w, 1, 1));
          vertexData.push_back(packVertex(_x + 1, _y, _z, NZ, w, 1, 0));
          vertexData.push_back(packVertex(_x, _y, _z, NZ, w, 0, 0));
          vertexData.push_back(packVertex(_x, _y + 1, _z, NZ, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z, NZ, w, 1, 1));
        }

        // add a face if +z is transparent
        if(isTransparent(get(_x, _y, _z + 1, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][5]; // get texture coordinates

          vertexData.push_back(packVertex(_x, _y, _z + 1, PZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y, _z + 1, PZ, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PZ, w, 1, 1));
          vertexData.push_back(packVertex(_x, _y, _z + 1, PZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PZ, w, 1, 1));
          vertexData.push_back(packVertex(_x, _y + 1, _z + 1, PZ, w, 0, 1));
        }

        // add a face if -y is transparent
        if(isTransparent(get(_x, _y - 1, _z, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][3]; // get texture coordinates

          vertexData.push_back(packVertex(_x, _y, _z, NY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y, _z, NY, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y, _z + 1, NY, w, 1, 1));
          vertexData.push_back(packVertex(_x, _y, _z, NY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y, _z + 1, NY, w, 1, 1));
          vertexData.push_back(packVertex(_x, _y, _z + 1, NY, w, 0, 1));
        }

        // add a face if +y is transparent
        if(isTransparent(get(_x, _y + 1, _z, px, nx, py, ny, pz, nz))) {
          w = BLOCKS[block][2]; // get texture coordinates

          vertexData.push_back(packVertex(_x, _y + 1, _z, PY, w, 0, 1));
          vertexData.push_back(packVertex(_x, _y + 1, _z + 1, PY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PY, w, 1, 0));
          vertexData.push_back(packVertex(_x, _y + 1, _z, PY, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PY, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z, PY, w, 1, 1));
        }
      }
    }
  }

  elements = (uint)vertexData.size(); // set number of vertices
  meshChanged = true;

#ifdef PRINT_TIMING
  printf("created chunk with %d vertices: ", elements);
#endif

  return true;
}

void Chunk::draw() {
  bufferMesh();

  vao->bind();
  GL::drawArrays(elements);
}

// if the chunk's mesh has been modified then send the new data to opengl (TODO: don't create a new buffer, just reuse the old one)
void Chunk::bufferMesh() {
  // if the mesh has not been modified then don't bother
  if(!meshChanged) {
    return;
  }

#ifdef PRINT_TIMING
  Timer timer;
#endif

  if(vao == nullptr) {
    vao = new GL::VAO();
  }

  GL::Buffer<GL::ARRAY>* vbo = new GL::Buffer<GL::ARRAY>();

  vao->bind();

  vbo->data(elements * sizeof(int), vertexData.data());
  vao->attribI(0, 1, GL::INT);

  GL::VAO::unbind();
  delete vbo;

  vertexData.clear();
  vertexData.shrink_to_fit();

  meshChanged = false;

#ifdef PRINT_TIMING
  printf("buffered chunk mesh: %zuB ", elements * sizeof(int));
#endif
}

inline block_t Chunk::get(uint8_t _x, uint8_t _y, uint8_t _z, const std::shared_ptr<Chunk>& px, const std::shared_ptr<Chunk>& nx, const std::shared_ptr<Chunk>& py, const std::shared_ptr<Chunk>& ny,
                          const std::shared_ptr<Chunk>& pz, const std::shared_ptr<Chunk>& nz) {
  if(_x < 0) { // gets block from -x neighbor
    return nx->blocks[blockIndex(CHUNK_SIZE + _x, _y, _z)];
  }

  if(_x >= CHUNK_SIZE) { // gets block from +x neighbor
    return px->blocks[blockIndex(_x % CHUNK_SIZE, _y, _z)];
  }

  if(_y < 0) { // gets block from -y neighbor
    return ny->blocks[blockIndex(_x, CHUNK_SIZE + _y, _z)];
  }

  if(_y >= CHUNK_SIZE) { // gets block from +y neighbor
    return py->blocks[blockIndex(_x, _y % CHUNK_SIZE, _z)];
  }

  if(_z < 0) { // gets block from -z neighbor
    return nz->blocks[blockIndex(_x, _y, CHUNK_SIZE + _z)];
  }

  if(_z >= CHUNK_SIZE) { // gets block from +z neighbor
    return pz->blocks[blockIndex(_x, _y, _z % CHUNK_SIZE)];
  }

  return blocks[blockIndex(_x, _y, _z)];
}

block_t Chunk::get(uint8_t _x, uint8_t _y, uint8_t _z) {
  return blocks[blockIndex(_x, _y, _z)];
}

void Chunk::set(uint8_t _x, uint8_t _y, uint8_t _z, block_t block) {
  blocks[blockIndex(_x, _y, _z)] = block;
  changed = true;
}
