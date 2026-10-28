#pragma once
#include "cheats.h"

namespace dlc {

enum Mode { MODE_NONE, MODE_PRISON, MODE_BOZAK };

inline Mode mode() {
    for (uintptr_t d : game::g.prison_data)
        if (game::rdv<uintptr_t>(d) == game::g.vt_prison_data) return MODE_PRISON;
    return MODE_NONE;
}

}
