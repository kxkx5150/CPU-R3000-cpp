#include "../core/pcsx.h"

PCSX *g_pcsx = nullptr;

int main(int argc, char *argv[])
{
    PCSX *pcsx = new PCSX();
    g_pcsx     = pcsx;
    return pcsx->mainloop(argv[1]);
}
