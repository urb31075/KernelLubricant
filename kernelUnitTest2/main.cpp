#include <cstdlib>
#include <iostream>
#include <unistd.h>
using namespace std;
int main(int argc, char** argv) 
{
    //EXIT_SUCCESS and EXIT_FAILURE
    cout << "Kernel test 2 - Start!" << endl;
    usleep(3000000);      
    cout << "Kernel test 2 - Ok!" << endl;
    return EXIT_SUCCESS;
}

