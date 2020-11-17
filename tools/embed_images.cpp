#include <stdio.h>
#include <string>
#include <algorithm>

void compileImagePNGtoBinary(const char* name, const char* filename, const char* output) {
  FILE* file = fopen(filename, "rb");

  if(file == NULL) {
    fprintf(stderr, "error opening source file\n");
    exit(0);
  }

  FILE* out = fopen(output, "w");

  if(out == NULL) {
    fprintf(stderr, "error opening output file\n");
    exit(0);
  }

  unsigned char buffer[32];
  size_t count;
  fprintf(out, "#ifndef IMAGE_%s\n#define IMAGE_%s\nstatic unsigned char IMAGE_%s_BYTES[] = {", name, name, name);

  while(!feof(file)) {
    count = fread(buffer, 1, 32, file);

    for(size_t i = 0; i < count; i++) {
      fprintf(out, "0x%02X,", buffer[i]);
    }
  }

  fprintf(out, "};\n#endif");
  fclose(file);
  fclose(out);
};

int main(int argc, char** argv) {
  if(argc != 2) {
    printf("you must provide a single filename\n");
    return 0;
  }

  std::string name = argv[1];
  std::string nameUpper = name;
  transform(nameUpper.begin(), nameUpper.end(), nameUpper.begin(), ::toupper);

  printf("converting image to binary: %s\n", name.c_str());
  compileImagePNGtoBinary(nameUpper.c_str(), (std::string("res/") + name + std::string(".png")).c_str(), (std::string("embed/res/") + name + std::string(".h")).c_str());

  return 0;
}
