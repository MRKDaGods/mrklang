#include <iostream>

#include "common/types.h"
#include "core/core.h"

using namespace mrklang;

int main() {
    std::cout << "mrklang codedom alpha\n";

    Vec<Str> sourceFilenames = { /*"examples/hello.mrk", "examples/runtime.mrk" */ "examples/main.mrk" };
    Core core(sourceFilenames);
    int result = core.build();

    return result;
}
