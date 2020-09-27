#ifndef COMMON_H_
#define COMMON_H_

#include <map>

#define CATCH_OPENGL_ERROR {GLenum err; while((err = glGetError()) != GL_NO_ERROR){fprintf(stderr, "[OpenGL Error] %s:%d (%s): %#8x\n", __FILE__, __LINE__, __func__, err);}}

extern std::string stackTraceName;
extern std::string stackTraceFile;
extern unsigned int stackTraceLine;
extern std::string stackTraceFunc;
void stackTracePush(const char* name, const char* file, unsigned int line, const char* func);

#define STACK_TRACE_PUSH(x) stackTracePush(x, __FILE__, __LINE__, __func__);

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

#endif
