#include "chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <glfw/glfw3.h>

#include "noise.h"
#include "blocks.h"
#include "chunk_manager.h"
#include "gl/buffer.h"

#define sign(_x) ({ __typeof__(_x) _xx = (_x);\
  ((__typeof__(_x)) ( (((__typeof__(_x)) 0) < _xx) - (_xx < ((__typeof__(_x)) 0))));})

#define VOID_BLOCK 0 // block id if there is no neighbor for block
// #define PRINT_TIMING

enum NORMAL_FACES : uint8_t{
  PY = 0,
  NY,
  PX,
  NX,
  PZ,
  NZ
};

inline float lerp(float a, float b, float t){
  return a * (1.0f - t) + b * t;
}

inline ushort blockIndex(uint8_t x, uint8_t y, uint8_t z){
  return x | (y << 5) | (z << 10);
}

// use magic numbers >.> to check if a block ID is transparent
inline bool isTransparent(block_t block){
  return block == 0 || block == 6;
}

/*
  x y z 6 bits
  normal 3 bits
  textureId 8 bits
  texX texY 1 bit
*/
inline int packVertex(uint8_t x, uint8_t y, uint8_t z, NORMAL_FACES normal, uint8_t textureId, uint8_t texX, uint8_t texY){
  return x | (y << 6) | (z << 12) | (normal << 18) | (textureId << 21) | (texX << 29) | (texY << 30);
}

Chunk::Chunk(int _x, int _y, int _z){
#ifdef PRINT_TIMING
  double start = glfwGetTime();
  unsigned short count = 0;
#endif

  blocks = (block_t*)malloc(CHUNK_SIZE_CUBED * sizeof(block_t));
  if(blocks == NULL){
    fprintf(stderr, ";-;\n");
    exit(-1);
  }

  vao = nullptr;
  elements = 0;
  changed = false;
  empty = true;
  meshChanged = false;

  x = _x;
  y = _y;
  z = _z;

  model = glm::translate(glm::mat4(1.0f), glm::vec3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE));

  int chunkX, chunkZ, height, realHeight;
  float f, biomeHeight, e;
  uint8_t dx, dy, dz, thickness;
  block_t block;

  for(dx = 0; dx < CHUNK_SIZE; dx++){
    for(dz = 0; dz < CHUNK_SIZE; dz++){
      chunkX = x * CHUNK_SIZE + dx;
      chunkZ = z * CHUNK_SIZE + dz;

      biomeHeight = simplex2(chunkX * 0.003f, chunkZ * 0.003f, 2, 0.6f, 1.5f);
      e = lerp(1.3f, 0.2f, (biomeHeight - 1.0f) / 2.0f);

      f = simplex2(chunkX * 0.002f, chunkZ * 0.002f, 6, 0.6f, 1.5f);
      height = pow((f + 1) / 2 + 1, 9) * e;
      realHeight = height - y * CHUNK_SIZE;

      for(dy = 0; dy < CHUNK_SIZE; dy++){
        thickness = realHeight - dy;
        block = height < 15 && thickness <= 10 ? 8 : height < 18 && thickness <= 3 ? 5 : height >= 160 && thickness <= 7 ? 7 : thickness == 1 ? 1 : thickness <= 6 ? 3 : 2;
        if(block == 8 && height < 14){
          height = 14;
          realHeight = height - y * CHUNK_SIZE;
        }

        block = dy < realHeight ? block : VOID_BLOCK;
        blocks[blockIndex(dx, dy, dz)] = block;

        if(empty && block != VOID_BLOCK){
          changed = true;
          empty = false;
        }

#ifdef PRINT_TIMING
        if(dy < realHeight){
          count++;
        }
#endif
      }
    }
  }

#ifdef PRINT_TIMING
  // printf("chunk gen: %.4fms with %d blocks\n", (glfwGetTime() - start) * 1000.0, count);
#endif
}

Chunk::~Chunk(){
  if(vao != nullptr){
    delete vao;
  }

  // delete the stored data
  free(blocks);
  blocks = NULL;
}

// update the chunk
bool Chunk::update(){
  // if the chunk does not need to remesh then stop
  if(!changed || blocks == NULL){
    return false;
  }

#ifdef PRINT_TIMING
  double start = glfwGetTime();
#endif

  STACK_TRACE_PUSH("chunk neighbors")

  // get chunk neighbors
  std::shared_ptr<Chunk> px = ChunkManager::get({x + 1, y, z});
  std::shared_ptr<Chunk> nx = ChunkManager::get({x - 1, y, z});
  std::shared_ptr<Chunk> py = ChunkManager::get({x, y + 1, z});
  std::shared_ptr<Chunk> ny = ChunkManager::get({x, y - 1, z});
  std::shared_ptr<Chunk> pz = ChunkManager::get({x, y, z + 1});
  std::shared_ptr<Chunk> nz = ChunkManager::get({x, y, z - 1});

  if(!px || !nx || !py || !ny || !pz || !nz){
    return false;
  }

  STACK_TRACE_PUSH("update chunk")

  // updating is taken care of - reset flag
  changed = false;

  uint8_t w, _x, _y, _z;
  block_t block;

  for(_z = 0; _z < CHUNK_SIZE; _z++){
    for(_x = 0; _x < CHUNK_SIZE; _x++){
      for(_y = 0; _y < CHUNK_SIZE; _y++){
        block = blocks[blockIndex(_x, _y, _z)];

        if(block == VOID_BLOCK){
          continue;
        }

        // add a face if -x is transparent
        if(isTransparent(get(_x - 1, _y, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][0]; // get texture coordinates

          vertexData.push_back(packVertex(_x    , _y    , _z    , NX, w, 0, 0));
          vertexData.push_back(packVertex(_x    , _y + 1, _z + 1, NX, w, 1, 1));
          vertexData.push_back(packVertex(_x    , _y + 1, _z    , NX, w, 0, 1));
          vertexData.push_back(packVertex(_x    , _y    , _z    , NX, w, 0, 0));
          vertexData.push_back(packVertex(_x    , _y    , _z + 1, NX, w, 1, 0));
          vertexData.push_back(packVertex(_x    , _y + 1, _z + 1, NX, w, 1, 1));
        }

        // add a face if +x is transparent
        if(isTransparent(get(_x + 1, _y, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][1]; // get texture coordinates

          vertexData.push_back(packVertex(_x + 1, _y    , _z    , PX, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PX, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y    , _z + 1, PX, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y    , _z    , PX, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z    , PX, w, 1, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PX, w, 0, 1));
        }

        // add a face if -z is transparent
        if(isTransparent(get(_x, _y, _z - 1, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][4]; // get texture coordinates

          vertexData.push_back(packVertex(_x    , _y    , _z    , NZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z    , NZ, w, 1, 1));
          vertexData.push_back(packVertex(_x + 1, _y    , _z    , NZ, w, 1, 0));
          vertexData.push_back(packVertex(_x    , _y    , _z    , NZ, w, 0, 0));
          vertexData.push_back(packVertex(_x    , _y + 1, _z    , NZ, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z    , NZ, w, 1, 1));
        }

        // add a face if +z is transparent
        if(isTransparent(get(_x, _y, _z + 1, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][5]; // get texture coordinates

          vertexData.push_back(packVertex(_x    , _y    , _z + 1, PZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y    , _z + 1, PZ, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PZ, w, 1, 1));
          vertexData.push_back(packVertex(_x    , _y    , _z + 1, PZ, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PZ, w, 1, 1));
          vertexData.push_back(packVertex(_x    , _y + 1, _z + 1, PZ, w, 0, 1));
        }

        // add a face if -y is transparent
        if(isTransparent(get(_x, _y - 1, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][3]; // get texture coordinates

          vertexData.push_back(packVertex(_x    , _y    , _z    , NY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y    , _z    , NY, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y    , _z + 1, NY, w, 1, 1));
          vertexData.push_back(packVertex(_x    , _y    , _z    , NY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y    , _z + 1, NY, w, 1, 1));
          vertexData.push_back(packVertex(_x    , _y    , _z + 1, NY, w, 0, 1));
        }

        // add a face if +y is transparent
        if(isTransparent(get(_x, _y + 1, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][2]; // get texture coordinates

          vertexData.push_back(packVertex(_x    , _y + 1, _z    , PY, w, 0, 1));
          vertexData.push_back(packVertex(_x    , _y + 1, _z + 1, PY, w, 0, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PY, w, 1, 0));
          vertexData.push_back(packVertex(_x    , _y + 1, _z    , PY, w, 0, 1));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z + 1, PY, w, 1, 0));
          vertexData.push_back(packVertex(_x + 1, _y + 1, _z    , PY, w, 1, 1));
        }
      }
    }
  }

  elements = (uint)vertexData.size(); // set number of vertices
  meshChanged = true; // set mesh has changed flag

#ifdef PRINT_TIMING
  printf("created chunk with %d vertices in %.4fms\n", elements, (glfwGetTime() - start) * 1000.0);
#endif

  return true;
}

void Chunk::draw(){
  bufferMesh();

  vao->bind();
  GL::drawArrays(elements);
}

// if the chunk's mesh has been modified then send the new data to opengl (TODO: don't create a new buffer, just reuse the old one)
void Chunk::bufferMesh(){
  // if the mesh has not been modified then don't bother
  if(!meshChanged){
    return;
  }

#ifdef PRINT_TIMING
  double start = glfwGetTime();
#endif

  if(vao == nullptr){
    vao = new GL::VAO();
  }

  GL::Buffer<GL::ARRAY>* vbo = new GL::Buffer<GL::ARRAY>();

  vao->bind();

  vbo->data(elements * sizeof(int), vertexData.data());
  vao->attribI(0, 1, GL_INT);

  GL::VAO::unbind();
  delete vbo;

  vertexData.clear();
  vertexData.shrink_to_fit();

  meshChanged = false;

#ifdef PRINT_TIMING
  printf("buffered chunk mesh in %.4fms\n", (glfwGetTime() - start) * 1000.0);
#endif
}

inline block_t Chunk::get(uint8_t _x, uint8_t _y, uint8_t _z, std::shared_ptr<Chunk> px, std::shared_ptr<Chunk> nx, std::shared_ptr<Chunk> py, std::shared_ptr<Chunk> ny, std::shared_ptr<Chunk> pz, std::shared_ptr<Chunk> nz){
  if(_x < 0){
    return nx->blocks[blockIndex(CHUNK_SIZE + _x, _y, _z)];
  }else if(_x >= CHUNK_SIZE){
    return blocks[blockIndex(_x % CHUNK_SIZE, _y, _z)];
  }else if(_y < 0){
    return ny->blocks[blockIndex(_x, CHUNK_SIZE + _y, _z)];
  }else if(_y >= CHUNK_SIZE){
    return py->blocks[blockIndex(_x, _y % CHUNK_SIZE, _z)];
  }else if(_z < 0){
    return nz->blocks[blockIndex(_x, _y, CHUNK_SIZE + _z)];
  }else if(_z >= CHUNK_SIZE){
    return pz->blocks[blockIndex(_x, _y, _z % CHUNK_SIZE)];
  }

  return blocks[blockIndex(_x, _y, _z)];
}

void Chunk::set(uint8_t _x, uint8_t _y, uint8_t _z, block_t block){
  blocks[blockIndex(_x, _y, _z)] = block;
  changed = true;
}
