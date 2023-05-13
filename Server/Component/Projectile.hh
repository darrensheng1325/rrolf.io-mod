#pragma once

#include <cstdint>

#include <Shared/Entity.hh>

namespace bc
{
    class BinaryCoder;
}

namespace app
{
    class Simulation;
}

namespace app::component
{
    class Projectile
    {
        Simulation *m_Simulation;

    public:
        using Type = Projectile;

        Entity m_Parent;
        int32_t m_ShootDelay = 0;
        uint32_t m_TicksUntilDeath = 0;
        uint64_t m_State = 0;

        Projectile(Entity, Simulation *);
        ~Projectile();

        void Reset();

        static void Write(bc::BinaryCoder &, Type const &, bool);
    };
}