#ifndef COMMON_H_
#define COMMON_H_

#include <map>

#define CATCH_OPENGL_ERROR {GLenum err; while((err = glGetError()) != GL_NO_ERROR){fprintf(stderr, "[OpenGL Error] %s:%d (%s): %#8x\n", __FILE__, __LINE__, __func__, err);}}

struct vec3i{
  int x;
  int y;
  int z;
};

inline bool const operator==(const vec3i& l, const vec3i& r){
	return l.x == r.x && l.y == r.y && l.z == r.z;
};

inline bool const operator!=(const vec3i& l, const vec3i& r){
	return l.x != r.x || l.y != r.y || l.z != r.z;
};

inline bool const operator<(const vec3i& l, const vec3i& r){
	if(l.x < r.x){
    return true;
  }else if(l.x > r.x){
    return false;
  }

	if(l.y < r.y){
    return true;
  }else if(l.y > r.y){
    return false;
  }

	if(l.z < r.z){
    return true;
  }else if(l.z > r.z){
    return false;
  }

	return false;
};

class Chunk;
extern std::map<vec3i, Chunk*> chunks;
typedef std::map<vec3i, Chunk*>::iterator chunk_it;

Chunk* getChunk(vec3i pos);

#endif
