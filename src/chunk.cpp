#include "chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>

#include "common.h"
#include "noise.h"
#include "blocks.h"
#include "chunk_manager.h"

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

inline unsigned short blockIndex(uint8_t x, uint8_t y, uint8_t z){
  return x | (y << 5) | (z << 10);
}

inline void byte4Set(uint8_t x, uint8_t y, uint8_t z, uint8_t w, byte4 dest){
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

inline void byte3Set(uint8_t x, uint8_t y, uint8_t z, byte3 dest){
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
}

// use magic numbers >.> to check if a block ID is transparent
inline bool isTransparent(block_t block){
  switch(block){
    case 0:
    case 6:
      return true;
    default:
      return false;
  }
}

unsigned int makeBuffer(GLenum target, GLsizei size, const void* data){
  unsigned int buffer;

  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, GL_STATIC_DRAW);

  return buffer;
}

/*
  x y z 6 bits
  normal 3 bits
  textureId 8 bits
  texX 1 bit
  texY 1 bit
*/
inline int packVertex(uint8_t x, uint8_t y, uint8_t z, NORMAL_FACES normal, uint8_t textureId, uint8_t texX, uint8_t texY){
  return x | (y << 6) | (z << 12) | (normal << 18) | (textureId << 21) | (texX << 29) | (texY << 30);
}

Chunk::Chunk(int _x, int _y, int _z){
#ifdef PRINT_TIMING
  double start = glfwGetTime();
#endif

  blocks = (block_t*)malloc(CHUNK_SIZE_CUBED * sizeof(block_t));
  if(blocks == NULL){
    fprintf(stderr, ";-;\n");
  }
  vao = 0;
  elements = 0;
  changed = false;
  empty = true;
  meshChanged = false;

  x = _x;
  y = _y;
  z = _z;

  model = glm::translate(glm::mat4(1.0f), glm::vec3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE));

#ifdef PRINT_TIMING
  unsigned short count = 0;
#endif

  int dx, dz, cx, cz, h, rh;
  float f, biomeHeight, e;
  uint8_t dy, thickness;
  block_t block;

  for(dx = 0; dx < CHUNK_SIZE; dx++){
    for(dz = 0; dz < CHUNK_SIZE; dz++){
      cx = x * CHUNK_SIZE + dx;
      cz = z * CHUNK_SIZE + dz;

      biomeHeight = simplex2(cx * 0.003f, cz * 0.003f, 2, 0.6f, 1.5f);
      e = lerp(1.3f, 0.2f, (biomeHeight - 1.0f) / 2.0f);

      f = simplex2(cx * 0.002f, cz * 0.002f, 6, 0.6f, 1.5f);
      h = pow((f + 1) / 2 + 1, 9) * e;
      rh = h - y * CHUNK_SIZE;

      for(dy = 0; dy < CHUNK_SIZE; dy++){
        thickness = rh - dy;
        block = h < 15 && thickness <= 10 ? 8 : h < 18 && thickness <= 3 ? 5 : h >= 160 && thickness <= 7 ? 7 : thickness == 1 ? 1 : thickness <= 6 ? 3 : 2;
        if(block == 8 && h < 14){
          h = 14;
          rh = h - y * CHUNK_SIZE;
        }

        block = dy < rh ? block : VOID_BLOCK;
        blocks[blockIndex(dx, dy, dz)] = block;

        if((!changed || empty) && block > 0){
          changed = true;
          empty = false;
        }

#ifdef PRINT_TIMING
        if(dy < h){
          count++;
        }
#endif
      }
    }
  }

#ifdef PRINT_TIMING
  printf("chunk gen: %.2fms with %d blocks\n", (glfwGetTime() - start) * 1000.0, count);
#endif
}

Chunk::~Chunk(){
  // delete the vertex array
  if(vao != 0){
    glDeleteVertexArrays(1, &vao);
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

  if(!px || !nx || !py || !ny || !pz || !nz){ // plz work 🥺
    printf("a\n");
    return false;
  }

  STACK_TRACE_PUSH("update chunk")

  // updating is taken care of - reset flag
  changed = false;

  uint8_t w;
  block_t block;

  for(uint8_t _z = 0; _z < CHUNK_SIZE; _z++){
    for(uint8_t _x = 0; _x < CHUNK_SIZE; _x++){
      for(uint8_t _y = 0; _y < CHUNK_SIZE; _y++){
        block = blocks[blockIndex(_x, _y, _z)];

        if(!block){
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

  elements = (int)vertexData.size(); // set number of vertices
  meshChanged = true; // set mesh has changed flag

#ifdef PRINT_TIMING
  printf("created chunk with %d vertices in %.2fms\n", i, (glfwGetTime() - start) * 1000.0);
#endif

  return true;
}

void Chunk::draw(){
  bufferMesh();

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, elements); CATCH_OPENGL_ERROR
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

  if(vao == 0){
    glGenVertexArrays(1, &vao);
  }

  glBindVertexArray(vao);

  unsigned int vertexDataBuffer = makeBuffer(GL_ARRAY_BUFFER, elements * sizeof(int), vertexData.data());
  glVertexAttribIPointer(0, 1, GL_INT, 0, 0);
  glEnableVertexAttribArray(0);

  glBindVertexArray(0);
  glDeleteBuffers(1, &vertexDataBuffer);

  vertexData.clear();
  vertexData.shrink_to_fit();

  meshChanged = false;

#ifdef PRINT_TIMING
  printf("buffered chunk mesh in %.2fms\n", (glfwGetTime() - start) * 1000.0);
#endif
}

block_t Chunk::get(uint8_t _x, uint8_t _y, uint8_t _z, std::shared_ptr<Chunk> px, std::shared_ptr<Chunk> nx, std::shared_ptr<Chunk> py, std::shared_ptr<Chunk> ny, std::shared_ptr<Chunk> pz, std::shared_ptr<Chunk> nz){
  if(_x < 0){
    return nx == nullptr ? VOID_BLOCK : nx->blocks[blockIndex(CHUNK_SIZE + _x, _y, _z)];
  }else if(_x >= CHUNK_SIZE){
    return px == nullptr ? VOID_BLOCK : px->blocks[blockIndex(_x % CHUNK_SIZE, _y, _z)];
  }else if(_y < 0){
    return ny == nullptr ? VOID_BLOCK : ny->blocks[blockIndex(_x, CHUNK_SIZE + _y, _z)];
  }else if(_y >= CHUNK_SIZE){
    return py == nullptr ? VOID_BLOCK : py->blocks[blockIndex(_x, _y % CHUNK_SIZE, _z)];
  }else if(_z < 0){
    return nz == nullptr ? VOID_BLOCK : nz->blocks[blockIndex(_x, _y, CHUNK_SIZE + _z)];
  }else if(_z >= CHUNK_SIZE){
    return pz == nullptr ? VOID_BLOCK : pz->blocks[blockIndex(_x, _y, _z % CHUNK_SIZE)];
  }

  return blocks[blockIndex(_x, _y, _z)];
}

void Chunk::set(uint8_t _x, uint8_t _y, uint8_t _z, block_t block){
  blocks[blockIndex(_x, _y, _z)] = block;
  changed = true;
}
