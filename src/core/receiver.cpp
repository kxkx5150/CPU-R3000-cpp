#include "pcsx.h"
#include <cstdlib>
#include "receiver.h"

extern PCSX *g_pcsx;

extern "C" void ReceiveMsg(int msgno)
{
    switch (msgno) {
        case MSG_EXIT: {
            g_pcsx->ClosePlugins();
            exit(1);
        } break;
    }
}
