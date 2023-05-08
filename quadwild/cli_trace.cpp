#include "trace.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <prefix>" << std::endl;
        return 1;
    }
    TraceMesh tm;
    bool success = trace(argv[1], tm);
    if (success) {
        std::cout << "success." << std::endl;
        return 0;
    } else {
        std::cout << "trace() failed." << std::endl;
        return 2;
    }
}
