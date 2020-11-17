#ifndef CHUNK_H_
#define CHUNK_H_

#include <vector>
#include <memory>

#include <glm/gtc/matrix_transform.hpp>

#include "gl/vao.h"

#include "common.h"
#include "blocks.h"

#define CHUNK_SIZE 32
#define CHUNK_SIZE_SQUARED 1024
#define CHUNK_SIZE_CUBED 32768

typedef int vec2i[2];

class Chunk{
public:
  int x;
  int y;
  int z;
  uint elements;
  bool changed;
  bool empty;
  glm::mat4 model;

  Chunk(int _x, int _y, int _z);
  ~Chunk();

  bool update();
  void draw();

  block_t get(uint8_t _x, uint8_t _y, uint8_t _z, const std::shared_ptr<Chunk>& px, const std::shared_ptr<Chunk>& nx, const std::shared_ptr<Chunk>& py, const std::shared_ptr<Chunk>& ny, const std::shared_ptr<Chunk>& pz, const std::shared_ptr<Chunk>& nz);
  block_t get(uint8_t _x, uint8_t _y, uint8_t _z);
  void set(uint8_t _x, uint8_t _y, uint8_t _z, block_t block);

private:
  block_t* blocks;
  GL::VAO* vao;
  bool meshChanged;
  std::vector<int> vertexData;

  inline void bufferMesh();
};

#endif
