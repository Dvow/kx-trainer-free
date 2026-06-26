#pragma once

#include <string>

namespace kx {

class VersionGate {
public:
    bool check();
    const std::string& lastMessage() const { return lastMessage_; }

private:
    std::string lastMessage_;
};

} // namespace kx
