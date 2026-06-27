#pragma once

#include "config.h"

#include <string>

namespace kx {

void normalizeKeyChord(Config::KeyChord& chord);
void addKeyToChord(Config::KeyChord& chord, int vk);
std::string chordLabel(const Config::KeyChord& chord);

bool isBindableKey(int vk);
bool chordHeld(const Config::KeyChord& binding);
Config::KeyChord readHeldBindableKeys();
bool anyInputHeld();

} // namespace kx
