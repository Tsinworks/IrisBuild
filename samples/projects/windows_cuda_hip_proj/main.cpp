#include "test.h"

#include <vector>
#include <iostream>
int hip_main();
int cu_main();

int main(int argc, const char* argv[]) {
  hip_main();
  cu_main();
  return 0;
}