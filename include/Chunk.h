#ifndef CHUNK_H_
#define CHUNK_H_

#include <stdint.h>

#define CHUNK_SIZE 32
#define CHUNK_SIZE_SQUARED 1024
#define CHUNK_SIZE_CUBED 32768

typedef uint8_t byte4[4];
typedef uint8_t byte3[3];
typedef int vec2i[2];
typedef uint8_t block_t;

class Chunk{
public:
  int x;
  int y;
  int z;
  int elements;

  Chunk(int _x, int _y, int _z);
  ~Chunk();

  bool update();
  void draw();

private:
  block_t *blocks;
  unsigned int vao;
  bool changed;
  bool meshChanged;
  byte4* vertex;
  char* brightness;
  byte3* normal;
  float *texCoords;

  inline void bufferMesh();
  inline block_t get(int _x, int _y, int _z, Chunk* px, Chunk* nx, Chunk* py, Chunk* ny, Chunk* pz, Chunk* nz);
};

#endif
