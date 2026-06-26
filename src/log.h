#pragma once

#include <string>

namespace kx::Log {

void add(const std::string& message);
void clear();
void copyToClipboard();
void renderInline(float height = 0.f);

}
