#ifndef CHUNK_H_
#define CHUNK_H_

#include <stdint.h>

#include <GL/glew.h>

#define CHUNK_SIZE 32
#define CHUNK_SIZE_SQUARED 1024
#define CHUNK_SIZE_CUBED 32768

typedef GLbyte byte4[4];
typedef GLbyte byte3[3];
typedef int vec2i[2];
typedef uint8_t block_t;

class Chunk{
public:
  int x;
  int y;
  int z;

  Chunk(int x, int y, int z);
  ~Chunk();

  bool update();
  void draw();

private:
  block_t *blocks;
  unsigned int vao;
  int elements;
  bool changed;
  bool meshChanged;
  byte4* vertex;
  char* brightness;
  byte3* normal;
  float *texCoords;

  void bufferMesh();
  block_t get(int x, int y, int z);
};

#endif
