#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config::Config(const char* configPath) {
  path = configPath;
}

char* Config::getString(const char* varName) {
  FILE* file = fopen(path, "r");

  if(file == NULL) {
    fprintf(stderr, "%s: %s not found\n", __func__, path);
    return NULL;
  }

  char name[128];
  char val[128];
  char* ret = NULL;

  while(fscanf(file, "%127[^=]=%127[^\n]%*c", name, val) == 2) {
    if(!strcmp(name, varName)) {
      ret = strdup(val);
      break;
    }
  }

  fclose(file);

  return ret;
}

int Config::getInt(const char* varName, int defaultValue) {
  char* temp = getString(varName);

  int ret = defaultValue;

  if(temp != NULL) {
    char* stop;
    ret = strtol(temp, &stop, 10);

    if(stop == temp) {
      ret = defaultValue;
    }

    free(temp);
  }

  if(ret < 1) {
    ret = 1;
  }

  printf("%s: %d\n", varName, ret);

  return ret;
}

bool Config::getBool(const char* varName, bool defaultValue) {
  char* temp = getString(varName);

  bool ret = defaultValue;;

  if(temp != NULL) {
    ret = strcmp(temp, "true") == 0;
    free(temp);
  }

  printf("%s: %s\n", varName, ret ? "yes" : "no");

  return ret;
}
