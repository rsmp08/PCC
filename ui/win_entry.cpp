#include <windows.h>

extern int main_gui();

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nShowCmd*/)
{
    return main_gui();
}
