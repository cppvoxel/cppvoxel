#include "Chunk.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <GL/glew.h>
#include <glfw/glfw3.h>

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

inline void byte4Set(GLbyte x, GLbyte y, GLbyte z, GLbyte w, byte4 dest){
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

inline void byte3Set(GLbyte x, GLbyte y, GLbyte z, byte3 dest){
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
}

// use magic numbers >.> to check if a block ID is transparent
inline unsigned char isTransparent(uint8_t block){
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
  elements = 0;
  changed = false;
  meshChanged = false;
  vao = -1;

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

        blocks[blockIndex(dx, dy, dz)] = dy < rh ? block : 0;
        if(!changed && blocks[blockIndex(dx, dy, dz)] > 0){
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
  glDeleteVertexArrays(1, &vao);

  // delete the stored data
  free(blocks);
}

// update the chunk
bool Chunk::update(){
  // if the chunk does not need to remesh then stop
  if(!changed || meshChanged){
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

  // allocate space for vertices, lighting, brightness, 
  vertex = (byte4*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(byte4));
  brightness = (char*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(char));
  normal = (byte3*)malloc(CHUNK_SIZE_CUBED * 2 * sizeof(byte3));
  texCoords = (float*)malloc(CHUNK_SIZE_CUBED * 4 * sizeof(float));

  for(uint8_t y = 0; y < CHUNK_SIZE; y++){
    for(uint8_t x = 0; x < CHUNK_SIZE; x++){
      for(uint8_t z = 0; z < CHUNK_SIZE; z++){
        block_t block = blocks[blockIndex(x, y, z)];

        if(!block){
          continue;
        }

        // texture coords
        uint8_t w;

        // add a face if -x is transparent
        if(isTransparent(get(x - 1, y, z))){
          w = BLOCKS[block][0]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x, y + 1, z, block, vertex[i++]);
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x, y, z + 1, block, vertex[i++]);
          byte4Set(x, y + 1, z + 1, block, vertex[i++]);

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
        if(isTransparent(get(x + 1, y, z))){
          w = BLOCKS[block][1]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x + 1, y, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);

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
        if(isTransparent(get(x, y, z - 1))){
          w = BLOCKS[block][4]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z, block, vertex[i++]);
          byte4Set(x + 1, y, z, block, vertex[i++]);
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x, y + 1, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z, block, vertex[i++]);

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
        if(isTransparent(get(x, y, z + 1))){
          w = BLOCKS[block][5]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x, y, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x, y, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x, y + 1, z + 1, block, vertex[i++]);

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
        if(isTransparent(get(x, y - 1, z))){
          w = BLOCKS[block][3]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x + 1, y, z, block, vertex[i++]);
          byte4Set(x + 1, y, z + 1, block, vertex[i++]);
          byte4Set(x, y, z, block, vertex[i++]);
          byte4Set(x + 1, y, z + 1, block, vertex[i++]);
          byte4Set(x, y, z + 1, block, vertex[i++]);

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
        if(isTransparent(get(x, y + 1, z))){
          w = BLOCKS[block][2]; // get texture coordinates
          // du = (w % TEXTURE_SIZE) * s; dv = (w / TEXTURE_SIZE) * s;
          du = w % TEXTURE_SIZE; dv = w / TEXTURE_SIZE;

          // set the vertex data for the face
          byte4Set(x, y + 1, z, block, vertex[i++]);
          byte4Set(x, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x, y + 1, z, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z + 1, block, vertex[i++]);
          byte4Set(x + 1, y + 1, z, block, vertex[i++]);

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
  bufferMesh();

  // don't draw if chunk has no mesh
  if(!elements){
    return;
  }

  // render only if all neighbors exist
  // if(chunk->px == NULL || chunk->nx == NULL || chunk->py == NULL || chunk->ny == NULL || chunk->pz == NULL || chunk->nz == NULL){
  //   return;
  // }

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, elements);
}

// if the chunk's mesh has been modified then send the new data to opengl (TODO: don't create a new buffer, just reuse the old one)
void Chunk::bufferMesh(){
  // if the mesh has not been modified then don't bother
  if(!meshChanged){
    return;
  }

#if defined DEBUG && defined PRINT_TIMING
  double start = glfwGetTime();
#endif

  if(vao == -1){
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
  free(brightness);
  free(normal);
  free(texCoords);

  meshChanged = false;

#if defined DEBUG && defined PRINT_TIMING
  printf("buffered chunk mesh in %.2fms\n", (glfwGetTime() - start) * 1000.0);
#endif
}

block_t Chunk::get(int _x, int _y, int _z){
  if(_x < 0){
    return VOID_BLOCK;
  }else if(_x >= CHUNK_SIZE){
    return VOID_BLOCK;
  }else if(_y < 0){
    return VOID_BLOCK;
  }else if(_y >= CHUNK_SIZE){
    return VOID_BLOCK;
  }else if(_z < 0){
    return VOID_BLOCK;
  }else if(_z >= CHUNK_SIZE){
    return VOID_BLOCK;
  }else{
    return blocks[blockIndex(_x, _y, _z)];
  }
}
