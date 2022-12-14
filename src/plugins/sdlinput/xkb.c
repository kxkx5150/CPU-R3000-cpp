/*
 * Copyright (c) 2009, Wei Mingzhi <whistler@openoffice.org>.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#include "pad.h"
#include "../../core/pcsx.h"
#include "../../core/receiver.h"

void InitKeyboard()
{
    g.PadState[0].KeyStatus = 0xFFFF;
    g.PadState[1].KeyStatus = 0xFFFF;
}

void DestroyKeyboard()
{
}
void CheckKeyboard()
{
    uint8_t     i, j, found;
    SDL_Event   evt;
    SDL_Keycode Key;

    while (SDL_PollEvent(&evt)) {
        switch (evt.type) {
            case SDL_QUIT:
                ReceiveMsg(MSG_EXIT);
                break;
            case SDL_KEYDOWN:
                Key   = evt.key.keysym.sym;
                found = 0;
                for (i = 0; i < 2; i++) {
                    for (j = 0; j < DKEY_TOTAL; j++) {
                        if (g.cfg.PadDef[i].KeyDef[j].Key == Key) {
                            found = 1;
                            g.PadState[i].KeyStatus &= ~(1 << j);
                        }
                    }
                }
                if (!found) {
                    g.KeyLeftOver = Key;
                }
                return;

            case SDL_KEYUP:
                Key   = evt.key.keysym.sym;
                found = 0;
                for (i = 0; i < 2; i++) {
                    for (j = 0; j < DKEY_TOTAL; j++) {
                        if (g.cfg.PadDef[i].KeyDef[j].Key == Key) {
                            found = 1;
                            g.PadState[i].KeyStatus |= (1 << j);
                        }
                    }
                }
                if (!found) {
                    g.KeyLeftOver = ((long)Key | 0x40000000);
                }
                break;
                /*
                                        case ClientMessage:
                                                xce = (XClientMessageEvent *)&evt;
                                                if (xce->message_type == wmprotocols && (Atom)xce->data.l[0] ==
                   wmdelwindow) {
                                                        // Fake an ESC key if user clicked the close button on window
                                                        g.KeyLeftOver = XK_Escape;
                                                        return;
                                                }
                                                break;
                                                */
        }
    }
}
