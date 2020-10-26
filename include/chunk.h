#ifndef CHUNK_H_
#define CHUNK_H_

#include <stdint.h>
#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

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
  bool changed;
  bool empty;
  glm::mat4 model;

  Chunk(int _x, int _y, int _z);
  ~Chunk();

  bool update();
  void draw();

  block_t get(uint8_t _x, uint8_t _y, uint8_t _z, std::shared_ptr<Chunk> px, std::shared_ptr<Chunk> nx, std::shared_ptr<Chunk> py, std::shared_ptr<Chunk> ny, std::shared_ptr<Chunk> pz, std::shared_ptr<Chunk> nz);
  block_t get(uint8_t _x, uint8_t _y, uint8_t _z){
    return get(_x, _y, _z, NULL, NULL, NULL, NULL, NULL, NULL);
  }
  void set(uint8_t _x, uint8_t _y, uint8_t _z, block_t block);

private:
  block_t *blocks;
  unsigned int vao;
  bool meshChanged;
  std::vector<int> vertexData;

  inline void bufferMesh();
};

#endif
