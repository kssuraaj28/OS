#include<stdint.h>

//TODO: The string can be hexa also
uint32_t atoi(char* string)
{
  uint32_t result = 0;
  char* ptr = string;
  while(*ptr)
  {
    result *=10;
    result += (uint32_t)*ptr - 0x30;
    ptr++;
  }
  return result;
}
