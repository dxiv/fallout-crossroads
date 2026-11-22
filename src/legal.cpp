#include "legal.hpp"

#include "resource.h"

namespace crossroads {

const wchar_t* legal_text() {
  return L"Fallout: Crossroads is an unofficial fan project.\r\n"
         L"\r\n"
         L"You must own legal PC copies of Fallout 3 (1.7.0.3-class) and "
         L"Fallout: New Vegas (1.4.0.525). This launcher only reads paths you "
         L"configure. It never copies, uploads, or redistributes Bethesda game "
         L"files, executables, or assets into git or public releases.\r\n"
         L"\r\n"
         L"Runtime may junction/hardlink your own Fallout 3 BSAs into your "
         L"New Vegas Data folder as CR_FO3_* so meshes resolve. Those files "
         L"remain part of your install.\r\n"
         L"\r\n"
         L"Fallout, Fallout 3, Fallout: New Vegas, Bethesda, and ZeniMax are "
         L"trademarks of their respective owners. This project is not "
         L"affiliated with or endorsed by Bethesda Softworks, ZeniMax Media, "
         L"Microsoft, Obsidian Entertainment, or Valve.\r\n"
         L"\r\n"
         L"Third-party mods are untested — see docs/mod-policy.md.\r\n"
         L"\r\n"
         L"Contributors' original code in this repository is offered under the "
         L"license in LICENSE.\r\n"
         L"\r\n"
         L"Do not use this project to play or share the games without owning "
         L"them.";
}

const wchar_t* credits_text() {
  return L"Fallout: Crossroads — Capital ↔ Mojave travel build\r\n"
         L"\r\n"
         L"Author: dxiv (https://github.com/dxiv)\r\n"
         L"\r\n"
         L"Phases: boot → Capital foothold → travel loop → coverage/spine → polish\r\n"
         L"\r\n"
         L"Reference decompilations (read-only, not linked into this binary):\r\n"
         L"  https://github.com/momusdigital/f3_decomp\r\n"
         L"  https://github.com/momusdigital/fnv_decomp\r\n"
         L"\r\n"
         L"See README.md, docs/ownership.md, docs/mod-policy.md.";
}

void show_legal_dialog(HWND owner) {
  MessageBoxW(owner, legal_text(), L"Legal — Fallout: Crossroads", MB_OK | MB_ICONINFORMATION);
}

}  // namespace crossroads
