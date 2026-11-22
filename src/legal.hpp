#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace crossroads {

void show_legal_dialog(HWND owner);

const wchar_t* legal_text();
const wchar_t* credits_text();

}  // namespace crossroads
