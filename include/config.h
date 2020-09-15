#ifndef CONFIG_H_
#define CONFIG_H_

class Config{
public:
  Config(const char* configPath);

  char* getString(const char* varName);
  int getInt(const char* varName, int defaultValue);
  bool getBool(const char* varName, bool defaultValue);

private:
  const char* path;
};

#endif
