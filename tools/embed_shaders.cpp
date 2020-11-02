#include <iostream>
#include <fstream>
#include <string>

void handleShader(std::ofstream& file, std::string name){
  std::cout << "embeding shader: " << name << "\n";
  std::ifstream vertexFile, fragmentFile;

  vertexFile.open(std::string("shaders/") + name + std::string("/vertex.glsl"));
  fragmentFile.open(std::string("shaders/") + name + std::string("/fragment.glsl"));

  std::string vertexSource, fragmentSource;
  std::getline(vertexFile, vertexSource, '\0');
  std::getline(fragmentFile, fragmentSource, '\0');

  file << "const ShaderSource " << name << " = {\n"
       << "R\"("
       << vertexSource
       << ")\",\n\n"
       << "R\"("
       << fragmentSource
       << ")\"\n"
       << "};\n\n";

  vertexFile.close();
  fragmentFile.close();
}

int main(){
  std::ofstream file;
  file.open("embed/shaders.h");

  file << "#ifndef SHADERS_H_\n"
       << "#define SHADERS_H_\n\n"
       << "#include \"gl/shader_source.h\"\n\n"
       << "namespace GL{\n"
       << "namespace Shaders{\n\n";

  std::ifstream listFile;
  listFile.open("shaders/list.txt");
  if(!listFile){
    std::cout << "ERROR: could not open shader list file\n";
    return -1;
  }

  std::string path;
  while(std::getline(listFile, path, '\n')){
    handleShader(file, path);
  }

  listFile.close();

  file << "}\n"
       << "}\n\n"
       << "#endif\n";
  file.close();

  return 0;
}
