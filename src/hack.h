#pragma once

#include "memory.h"

#include <windows.h>
#include <cstddef>

namespace kx {

class Hack {
public:
    bool initialize();
    void tick();

    void setFog(bool enabled);
    void setObjectClipping(bool enabled);
    void setFullStrafe(bool enabled);
    void setSprint(bool enabled);
    void setSuperSprint(bool enabled);
    void setInvisibility(bool enabled);
    void setWallClimb(bool enabled);
    void setClipping(bool enabled);
    void setFly(bool enabled);
    void savePosition();
    void loadPosition();

    bool isFogEnabled() const { return m_fogEnabled; }
    bool isObjectClippingEnabled() const { return m_objectClippingEnabled; }
    bool isFullStrafeEnabled() const { return m_fullStrafeEnabled; }
    bool isSprintEnabled() const { return m_sprintEnabled; }
    bool isSuperSprintEnabled() const { return m_superSprintEnabled; }
    bool isInvisibilityEnabled() const { return m_invisibilityEnabled; }
    bool isWallClimbEnabled() const { return m_wallClimbEnabled; }
    bool isClippingEnabled() const { return m_clippingEnabled; }
    bool isFlyEnabled() const { return m_flyEnabled; }

private:
    Memory m_mem;

    uintptr_t m_baseSlot = 0;
    uintptr_t m_fogAddr = 0;
    uintptr_t m_objectClippingAddr = 0;
    uintptr_t m_fullStrafeAddr = 0;

    uintptr_t m_xAddr = 0;
    uintptr_t m_yAddr = 0;
    uintptr_t m_zAddr = 0;
    uintptr_t m_zHeight1Addr = 0;
    uintptr_t m_zHeight2Addr = 0;
    uintptr_t m_gravityAddr = 0;
    uintptr_t m_speedAddr = 0;
    uintptr_t m_wallClimbAddr = 0;

    float m_x = 0.f, m_y = 0.f, m_z = 0.f;
    float m_xSaved = 0.f, m_ySaved = 0.f, m_zSaved = 0.f;
    float m_savedSpeed = 0.f;
    bool m_positionSaved = false;

    bool m_fogEnabled = false;
    bool m_objectClippingEnabled = false;
    bool m_fullStrafeEnabled = false;
    bool m_sprintEnabled = false;
    bool m_superSprintEnabled = false;
    bool m_invisibilityEnabled = false;
    bool m_wallClimbEnabled = false;
    bool m_clippingEnabled = false;
    bool m_flyEnabled = false;

    void scanBase();
    void scanFeatures();
    void fail(const char* message);
    void refreshAddresses();

    uintptr_t scanRequired(const char* label, const char* pattern, const char* mask, unsigned offset = 0);

    void readPosition();
    void writePosition(float x, float y, float z);
};

}
