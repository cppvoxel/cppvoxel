#include "Chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>

#include "common.h"
#include "noise.h"
#include "blocks.h"

#define TEXTURE_SIZE 4
#define VOID_BLOCK 0 // block id if there is no neighbor for block
// #define PRINT_TIMING

inline unsigned short blockIndex(uint8_t x, uint8_t y, uint8_t z){
  return x | (y << 5) | (z << 10);
}

inline float halfPixelCorrection(float coord){
  coord *= (1.0f / TEXTURE_SIZE); // convert texture pos to uv coord
  return coord;
  // return (coord + 0.5f) / TEXTURE_SIZE;
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
inline unsigned char isTransparent(block_t block){
  switch(block){
    case 0:
    case 6:
      return 1;
    default:
      return 0;
  }
}

unsigned int makeBuffer(GLenum target, GLsizei size, const void* data){
  unsigned int buffer;

  glGenBuffers(1, &buffer);
  glBindBuffer(target, buffer);
  glBufferData(target, size, data, GL_STATIC_DRAW);

  return buffer;
}

Chunk::Chunk(int _x, int _y, int _z){
#if defined DEBUG && defined PRINT_TIMING
  double start = glfwGetTime();
#endif

  blocks = (block_t*)malloc(CHUNK_SIZE_CUBED * sizeof(block_t));
  vao = 0;
  elements = 0;
  changed = false;
  meshChanged = false;
  vertex = NULL;
  brightness = NULL;
  normal = NULL;
  texCoords = NULL;

  x = _x;
  y = _y;
  z = _z;

#if defined DEBUG && defined PRINT_TIMING
  unsigned short count = 0;
#endif

  for(uint8_t dx = 0; dx < CHUNK_SIZE; dx++){
    for(uint8_t dz = 0; dz < CHUNK_SIZE; dz++){
      int cx = x * CHUNK_SIZE + dx;
      int cz = z * CHUNK_SIZE + dz;

      float f = simplex2(cx * 0.003f, cz * 0.003f, 6, 0.6f, 1.5f);
      int h = pow((f + 1) / 2 + 1, 9);
      int rh = h - y * CHUNK_SIZE;

      for(uint8_t dy = 0; dy < CHUNK_SIZE; dy++){
        uint8_t thickness = rh - dy;
        uint8_t block = h < 15 && thickness <= 10 ? 8 : h < 18 && thickness <= 3 ? 5 : h >= 140 && thickness <= 7 ? 7 : thickness == 1 ? 1 : thickness <= 6 ? 3 : 2;
        if(block == 8 && h < 14){
          h = 14;
          rh = h - y * CHUNK_SIZE;
        }
        block = dy < rh ? block : 0;

        blocks[blockIndex(dx, dy, dz)] = block;
        if(!changed && block > 0){
          changed = true;
        }

#if defined DEBUG && defined PRINT_TIMING
        if(dy < h){
          count++;
        }
#endif
      }
    }
  }

#if defined DEBUG && defined PRINT_TIMING
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

  // delete buffers if needed
  if(vertex != NULL){
    free(vertex);
    vertex = NULL;
  }
  if(brightness != NULL){
    free(brightness);
    brightness = NULL;
  }
  if(normal != NULL){
    free(normal);
    normal = NULL;
  }
  if(texCoords != NULL){
    free(texCoords);
    texCoords = NULL;
  }
}

// update the chunk
bool Chunk::update(){
  // if the chunk does not need to remesh then stop
  if(!changed){
    return false;
  }

#if defined DEBUG && defined PRINT_TIMING
  double start = glfwGetTime();
#endif

  // updating is taken care of - reset flag
  changed = false;

  unsigned int i = 0; // vertex index
  unsigned int j = 0; // lighting and normal index
  unsigned int texCoord = 0;

  // texure coordinates
  float du, dv;
  float a = 0.0f;
  float b = 1.0f;
  uint8_t w;

  block_t block;

  // allocate space for vertices, lighting, brightness, 
  if(vertex == NULL){
    vertex = (byte4*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(byte4));
  }
  if(brightness == NULL){
    brightness = (char*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(char));
  }
  if(normal == NULL){
    normal = (byte3*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(byte3));
  }
  if(texCoords == NULL){
    texCoords = (float*)malloc(CHUNK_SIZE_CUBED * 4 * sizeof(float));
  }

  // get chunk neighbors
  Chunk* px = getChunk((vec3i){x + 1, y, z});
  Chunk* nx = getChunk((vec3i){x - 1, y, z});
  Chunk* py = getChunk((vec3i){x, y + 1, z});
  Chunk* ny = getChunk((vec3i){x, y - 1, z});
  Chunk* pz = getChunk((vec3i){x, y, z + 1});
  Chunk* nz = getChunk((vec3i){x, y, z - 1});

  // if(px == NULL || nx == NULL || py == NULL || ny == NULL || pz == NULL || nz == NULL){
  //   return false;
  // }

  for(uint8_t _y = 0; _y < CHUNK_SIZE; _y++){
    for(uint8_t _x = 0; _x < CHUNK_SIZE; _x++){
      for(uint8_t _z = 0; _z < CHUNK_SIZE; _z++){
        block = blocks[blockIndex(_x, _y, _z)];

        if(!block){
          continue;
        }

        // add a face if -x is transparent
        if(isTransparent(get(_x - 1, _y, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][0]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z + 1, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 0;
            byte3Set(-1, 0, 0, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }

        // add a face if +x is transparent
        if(isTransparent(get(_x + 1, _y, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][1]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x + 1, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 0;
            byte3Set(1, 0, 0, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }

        // add a face if -z is transparent
        if(isTransparent(get(_x, _y, _z - 1, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][4]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z, block, vertex[i++]);
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 1;
            byte3Set(0, 0, -1, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }

        // add a face if +z is transparent
        if(isTransparent(get(_x, _y, _z + 1, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][5]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z + 1, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 1;
            byte3Set(0, 0, 1, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }

        // add a face if -y is transparent
        if(isTransparent(get(_x, _y - 1, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][3]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y, _z + 1, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 2;
            byte3Set(0, -1, 0, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }

        // add a face if +y is transparent
        if(isTransparent(get(_x, _y + 1, _z, px, nx, py, ny, pz, nz))){
          w = BLOCKS[block][2]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(_x, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x, _y + 1, _z, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z + 1, block, vertex[i++]);
          byte4Set(_x + 1, _y + 1, _z, block, vertex[i++]);

          // set the brightness data for the face
          for(int k = 0; k < 6; k++){
            brightness[j] = 2;
            byte3Set(0, 1, 0, normal[j++]);
          }

          // set the texture data for the face
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(a + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(a + dv);
          texCoords[texCoord++] = halfPixelCorrection(b + du); texCoords[texCoord++] = halfPixelCorrection(b + dv);
        }
      }
    }
  }

  elements = i; // set number of vertices
  meshChanged = true; // set mesh has changed flag

#if defined DEBUG && defined PRINT_TIMING
  printf("created chunk with %d vertices in %.2fms\n", i, (glfwGetTime() - start) * 1000.0);
#endif

  return true;
}

void Chunk::draw(){
  // don't draw if chunk has no mesh
  if(!elements){
    return;
  }

  bufferMesh();

  // render only if all neighbors exist
  // if(chunk->px == NULL || chunk->nx == NULL || chunk->py == NULL || chunk->ny == NULL || chunk->pz == NULL || chunk->nz == NULL){
  //   return;
  // }

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, elements);
}

// if the chunk's mesh has been modified then send the new data to opengl (TODO: don't create a new buffer, just reuse the old one)
inline void Chunk::bufferMesh(){
  // if the mesh has not been modified then don't bother
  if(!meshChanged){
    return;
  }

#if defined DEBUG && defined PRINT_TIMING
  double start = glfwGetTime();
#endif

  if(vao == 0){
    glGenVertexArrays(1, &vao);
  }

  glBindVertexArray(vao);

  unsigned int vertexBuffer = makeBuffer(GL_ARRAY_BUFFER, elements * sizeof(*vertex), vertex);
  glVertexAttribPointer(0, 4, GL_BYTE, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  unsigned int brightnessBuffer = makeBuffer(GL_ARRAY_BUFFER, elements * sizeof(*brightness), brightness);
  glVertexAttribIPointer(1, 1, GL_BYTE, 0, 0);
  glEnableVertexAttribArray(1);

  unsigned int normalBuffer = makeBuffer(GL_ARRAY_BUFFER, elements * sizeof(*normal), normal);
  glVertexAttribPointer(2, 3, GL_BYTE, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);

  unsigned int texureBuffer = makeBuffer(GL_ARRAY_BUFFER, elements * 2 * sizeof(*texCoords), texCoords);
  glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(3);

  glBindVertexArray(0);
  glDeleteBuffers(1, &vertexBuffer);
  glDeleteBuffers(1, &brightnessBuffer);
  glDeleteBuffers(1, &normalBuffer);
  glDeleteBuffers(1, &texureBuffer);

  free(vertex);
  vertex = NULL;
  free(brightness);
  brightness = NULL;
  free(normal);
  normal = NULL;
  free(texCoords);
  texCoords = NULL;

  meshChanged = false;

#if defined DEBUG && defined PRINT_TIMING
  printf("buffered chunk mesh in %.2fms\n", (glfwGetTime() - start) * 1000.0);
#endif
}

inline block_t Chunk::get(int _x, int _y, int _z, Chunk* px, Chunk* nx, Chunk* py, Chunk* ny, Chunk* pz, Chunk* nz){
  if(_x < 0){
    return nx == NULL ? VOID_BLOCK : nx->blocks[blockIndex(CHUNK_SIZE + _x, _y, _z)];
  }else if(_x >= CHUNK_SIZE){
    return px == NULL ? VOID_BLOCK : px->blocks[blockIndex(_x % CHUNK_SIZE, _y, _z)];
  }else if(_y < 0){
    return ny == NULL ? VOID_BLOCK : ny->blocks[blockIndex(_x, CHUNK_SIZE + _y, _z)];
  }else if(_y >= CHUNK_SIZE){
    return py == NULL ? VOID_BLOCK : py->blocks[blockIndex(_x, _y % CHUNK_SIZE, _z)];
  }else if(_z < 0){
    return nz == NULL ? VOID_BLOCK : nz->blocks[blockIndex(_x, _y, CHUNK_SIZE + _z)];
  }else if(_z >= CHUNK_SIZE){
    return pz == NULL ? VOID_BLOCK : pz->blocks[blockIndex(_x, _y, _z % CHUNK_SIZE)];
  }

  return blocks[blockIndex(_x, _y, _z)];
}
