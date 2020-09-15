#ifndef BLOCKS_H_
#define BLOCKS_H_

const int BLOCKS[256][6] = {
  // w => (left, right, top, bottom, front, back) tiles
  {0, 0, 0, 0, 0, 0}, // 0 - empty
  {4, 4, 8, 0, 4, 4}, // 1 - grass
  {1, 1, 1, 1, 1, 1}, // 2 - stone
  {0, 0, 0, 0, 0, 0}, // 3 - dirt
  {2, 2, 2, 2, 2, 2}, // 4 - bedrock
  {3, 3, 3, 3, 3, 3}, // 5 - sand
  {5, 5, 5, 5, 5, 5}, // 6 - glass
  {6, 6, 6, 6, 6, 6}, // 7 - snow
  {7, 7, 7, 7, 7, 7}, // 8 - water
  {9, 9, 10, 10, 9, 9}, // 9 - log
};

#endif
