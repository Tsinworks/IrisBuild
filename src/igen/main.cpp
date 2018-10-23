#include "parse/context.h"

Context gContext;

int main(int argc, char* argv[]) {
    if (argv[1]) {
        gContext.Load(argv[1]);
    }
    return 0;
}