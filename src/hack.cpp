#include "hack.h"
#include "config.h"
#include "constants.h"
#include "log.h"

#include <array>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace kx {
namespace {

namespace C = Constants;
namespace P = Constants::Patterns;
namespace S = Constants::Settings;

template <std::size_t N>
uintptr_t resolve(const Memory& mem, uintptr_t base, const std::array<unsigned, N>& chain) {
    return mem.ResolvePointerChain(base, chain);
}

void patchByte(const Memory& mem, uintptr_t addr, byte value) {
    if (mem.WriteProtected(addr, &value, sizeof(value)))
        return;
    static bool logged = false;
    if (!logged) {
        Log::add("WARN: Code patch write failed (memory protection). Feature may not apply.");
        logged = true;
    }
}

template <typename Fn>
void setFlag(bool& flag, bool enabled, Fn&& onDisable) {
    if (flag == enabled)
        return;
    flag = enabled;
    if (!enabled)
        onDisable();
}

}

void Hack::fail(const char* message) {
    Log::add(std::string("ERROR: ") + message);
    throw std::runtime_error(message);
}

uintptr_t Hack::scanRequired(const char* label, const char* pattern, const char* mask, unsigned offset) {
    const uintptr_t addr = m_mem.ScanMainModule(pattern, mask);
    if (!addr)
        fail((std::string("Failed to find ") + label + " pattern.").c_str());
    return addr + offset;
}

bool Hack::initialize() {
    Log::add("INFO: Starting KX Trainer initialization...");
    try {
        scanBase();
        scanFeatures();
        if (Config::hasSavedPosition()) {
            const Config::Position pos = Config::savedPosition();
            m_xSaved = pos.x;
            m_ySaved = pos.y;
            m_zSaved = pos.z;
            m_positionSaved = true;
        }
        Log::add("INFO: Initialization successful.");
        return true;
    } catch (const std::exception& e) {
        Log::add(std::string("ERROR: Initialization failed - ") + e.what());
        return false;
    }
}

void Hack::scanBase() {
    Log::add("INFO: Starting base address scan...");
    m_baseSlot = 0;

    uintptr_t slot = 0;
    for (int attempt = 1; attempt <= C::Scan::MAX_BASE_SCAN_ATTEMPTS && slot == 0; ++attempt) {
        Log::add("INFO: Scanning for base pattern... (Attempt " + std::to_string(attempt) + "/" +
            std::to_string(C::Scan::MAX_BASE_SCAN_ATTEMPTS) + ")");

        const uintptr_t match = m_mem.ScanMainModule(P::BASE, P::BASE_MASK);
        if (match != 0) {
            slot = match - C::Scan::POINTER_LOCATION_OFFSET;
            Log::add("INFO: Pattern matched. Pointer slot at " + to_hex(slot));
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(C::Scan::BASE_SCAN_RETRY_DELAY_MS));
    }

    if (slot == 0)
        fail("Failed to find base address pattern after maximum attempts.");

    m_baseSlot = slot;

    uintptr_t value = 0;
    if (m_mem.Read(slot, value) && value > C::Scan::BASE_ADDRESS_MIN_VALUE) {
        Log::add("INFO: Character pointer ready at " + to_hex(slot) + " (Value: " + to_hex(value) + ")");
    } else {
        Log::add("WARN: Character pointer not ready yet - load into the game world for movement features.");
    }
}

void Hack::scanFeatures() {
    Log::add("INFO: Scanning for feature patterns...");
    m_fogAddr = scanRequired("Fog", P::FOG, P::FOG_MASK, 0x3);
    m_objectClippingAddr = scanRequired("Object Clipping", P::OBJECT_CLIPPING, P::OBJECT_CLIPPING_MASK);
    m_fullStrafeAddr = scanRequired("Full Strafe", P::FULL_STRAFE, P::FULL_STRAFE_MASK, 0x2);
    Log::add("INFO: Feature patterns found successfully.");
}

void Hack::refreshAddresses() {
    if (m_baseSlot == 0)
        return;

    m_xAddr = resolve(m_mem, m_baseSlot, C::Chains::X);
    m_yAddr = resolve(m_mem, m_baseSlot, C::Chains::Y);
    m_zAddr = resolve(m_mem, m_baseSlot, C::Chains::Z);
    m_zHeight1Addr = resolve(m_mem, m_baseSlot, C::Chains::ZHeight1);
    m_zHeight2Addr = resolve(m_mem, m_baseSlot, C::Chains::ZHeight2);
    m_gravityAddr = resolve(m_mem, m_baseSlot, C::Chains::Gravity);
    m_speedAddr = resolve(m_mem, m_baseSlot, C::Chains::Speed);
    m_wallClimbAddr = resolve(m_mem, m_baseSlot, C::Chains::WallClimb);
}

void Hack::tick() {
    refreshAddresses();

    if (m_fogEnabled && m_fogAddr)
        patchByte(m_mem, m_fogAddr, S::NO_FOG_ON);
    if (m_objectClippingEnabled && m_objectClippingAddr)
        patchByte(m_mem, m_objectClippingAddr, S::OBJECT_CLIPPING_ON);
    if (m_fullStrafeEnabled && m_fullStrafeAddr)
        patchByte(m_mem, m_fullStrafeAddr, S::FULL_STRAFE_ON);
    if (m_invisibilityEnabled && m_zHeight1Addr)
        m_mem.Write(m_zHeight1Addr, S::INVISIBILITY_ON);
    if (m_wallClimbEnabled && m_wallClimbAddr)
        m_mem.Write(m_wallClimbAddr, S::WALLCLIMB_SPEED);
    if (m_clippingEnabled && m_zHeight2Addr)
        m_mem.Write(m_zHeight2Addr, S::CLIPPING_ON);
    if (m_flyEnabled && m_gravityAddr)
        m_mem.Write(m_gravityAddr, S::FLY_SPEED);
    if (m_superSprintEnabled && m_speedAddr)
        m_mem.Write(m_speedAddr, S::SUPER_SPRINT_SPEED);
    else if (m_sprintEnabled && m_speedAddr)
        m_mem.Write(m_speedAddr, S::SPRINT_SPEED);
}

void Hack::setFog(bool enabled) {
    setFlag(m_fogEnabled, enabled, [&] {
        if (m_fogAddr)
            patchByte(m_mem, m_fogAddr, S::NO_FOG_OFF);
    });
}

void Hack::setObjectClipping(bool enabled) {
    setFlag(m_objectClippingEnabled, enabled, [&] {
        if (m_objectClippingAddr)
            patchByte(m_mem, m_objectClippingAddr, S::OBJECT_CLIPPING_OFF);
    });
}

void Hack::setFullStrafe(bool enabled) {
    setFlag(m_fullStrafeEnabled, enabled, [&] {
        if (m_fullStrafeAddr)
            patchByte(m_mem, m_fullStrafeAddr, S::FULL_STRAFE_OFF);
    });
}

void Hack::setSprint(bool enabled) {
    setFlag(m_sprintEnabled, enabled, [&] {
        if (m_speedAddr && !m_superSprintEnabled)
            m_mem.Write(m_speedAddr, S::NORMAL_SPEED);
    });
}

void Hack::setSuperSprint(bool enabled) {
    if (m_superSprintEnabled == enabled)
        return;

    if (enabled && m_speedAddr) {
        float speed = 0.f;
        if (m_mem.Read(m_speedAddr, speed))
            m_savedSpeed = speed;
    }

    m_superSprintEnabled = enabled;

    if (!enabled && m_speedAddr) {
        const float restore = (m_savedSpeed >= S::NORMAL_SPEED - 0.1f) ? m_savedSpeed : S::NORMAL_SPEED;
        m_mem.Write(m_speedAddr, m_sprintEnabled ? S::SPRINT_SPEED : restore);
    }
}

void Hack::setInvisibility(bool enabled) {
    if (m_invisibilityEnabled == enabled)
        return;
    m_invisibilityEnabled = enabled;

    if (enabled || !m_zHeight1Addr)
        return;

    if (m_yAddr) {
        float y = 0.f;
        if (m_mem.Read(m_yAddr, y))
            m_mem.Write(m_yAddr, y + 3.f);
    }
    m_mem.Write(m_zHeight1Addr, S::INVISIBILITY_OFF);
}

void Hack::setWallClimb(bool enabled) {
    setFlag(m_wallClimbEnabled, enabled, [&] {
        if (m_wallClimbAddr)
            m_mem.Write(m_wallClimbAddr, S::WALLCLIMB_NORMAL_SPEED);
    });
}

void Hack::setClipping(bool enabled) {
    setFlag(m_clippingEnabled, enabled, [&] {
        if (m_zHeight2Addr)
            m_mem.Write(m_zHeight2Addr, S::CLIPPING_OFF);
    });
}

void Hack::setFly(bool enabled) {
    setFlag(m_flyEnabled, enabled, [&] {
        if (m_gravityAddr)
            m_mem.Write(m_gravityAddr, S::FLY_NORMAL_SPEED);
    });
}

void Hack::readPosition() {
    if (m_xAddr) m_mem.Read(m_xAddr, m_x);
    if (m_yAddr) m_mem.Read(m_yAddr, m_y);
    if (m_zAddr) m_mem.Read(m_zAddr, m_z);
}

void Hack::writePosition(float x, float y, float z) {
    if (m_xAddr) m_mem.Write(m_xAddr, x);
    if (m_yAddr) m_mem.Write(m_yAddr, y);
    if (m_zAddr) m_mem.Write(m_zAddr, z);
}

void Hack::savePosition() {
    refreshAddresses();
    if (!m_xAddr || !m_yAddr || !m_zAddr) {
        Log::add("WARN: Cannot save position yet - load into the game world first.");
        return;
    }

    readPosition();
    m_xSaved = m_x;
    m_ySaved = m_y;
    m_zSaved = m_z;
    m_positionSaved = true;
    Config::setSavedPosition(m_x, m_y, m_z);
    Config::save();
    Log::add("INFO: Position saved.");
}

void Hack::loadPosition() {
    if (!m_positionSaved)
        return;

    refreshAddresses();
    if (!m_xAddr || !m_yAddr || !m_zAddr)
        return;

    writePosition(m_xSaved, m_ySaved, m_zSaved);
    Log::add("INFO: Position loaded.");
}

}
