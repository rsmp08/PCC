#include <iostream>

extern int main_gui();

int main(int /*argc*/, char ** /*argv*/)
{
    std::cout << "Starting GUI (console attached)...\n";
    return main_gui();
}
