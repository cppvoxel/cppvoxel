#include "gl/buffer.h"

template <GL::BufferType T> GL::Buffer<T>::Buffer() {
  glGenBuffers(1, &handle);
}

template <GL::BufferType T> GL::Buffer<T>::~Buffer() {
  glDeleteBuffers(1, &handle);
}

template <GL::BufferType T>
void GL::Buffer<T>::bind() {
  glBindBuffer(T, handle);
}

template <GL::BufferType T>
void GL::Buffer<T>::unbind() {
  glBindBuffer(T, 0);
}

template <GL::BufferType T>
void GL::Buffer<T>::data(size_t size, const void* data) {
  bind();
  glBufferData(T, size, data, GL_STATIC_DRAW);
}

template class GL::Buffer<GL::ARRAY>;
template class GL::Buffer<GL::ELEMENT_ARRAY>;
