#include <Server/Component/Petal.hh>

#include <BinaryCoder/BinaryCoder.hh>
#include <BinaryCoder/NativeTypes.hh>

#include <Server/Simulation.hh>

namespace app::component
{
    Petal::Petal(Entity parent, Simulation *simulation)
        : m_Parent(parent),
          m_Simulation(simulation)
    {
    }

    Petal::~Petal()
    {
        Basic &basic = m_Simulation->Get<Basic>(m_Parent);
        if (!m_Simulation->HasEntity(basic.m_Owner))
            return;
        if (!m_Simulation->HasComponent<PlayerInfo>(basic.m_Owner))
            return;
        PlayerInfo &playerInfo = m_Simulation->Get<PlayerInfo>(basic.m_Owner);
        for (uint64_t i = 0; i < playerInfo.m_SlotCount; i++)
        {
            PlayerInfo::PetalSlot &slot = playerInfo.m_PetalSlots[i];
            for (uint64_t j = 0; j < slot.m_Petals.size(); j++)
            {
                PlayerInfo::Petal &playerInfoPetal = slot.m_Petals[j];
                if (m_Parent == playerInfoPetal.m_SimulationId)
                {
                    playerInfoPetal.m_IsDead = true;
                    playerInfoPetal.m_TicksUntilRespawn = slot.m_Data.m_ReloadTicks;
                }
            }
        }
    }

    void Petal::Reset()
    {
        m_State = 0;
    }

    void Petal::Write(bc::BinaryCoder &coder, Type const &entity, bool isCreation)
    {
        uint32_t state = isCreation ? 0b11 : entity.m_State;
        coder.Write<bc::VarUint>(state);

        if (state & 1)
            coder.Write<bc::VarUint>(entity.m_Id);
        if (state & 2)
            coder.Write<bc::VarUint>(entity.m_Rarity);
        if (state & 4)
            coder.Write<bc::Uint8>(entity.m_Shootable);
        if (state & 8)
            coder.Write<bc::Uint8>(entity.m_Independent);
    }

    uint32_t Petal::Id() const { return m_Id; }
    uint32_t Petal::Rarity() const { return m_Rarity; }
    bool Petal::Shootable() const { return m_Shootable; }
    bool Petal::Independent() const { return m_Independent; }

    void Petal::Id(uint32_t v)
    {
        if (v == m_Id)
            return;
        m_Id = v;
        m_State |= 1;
    }

    void Petal::Rarity(uint32_t v)
    {
        Life &life = m_Simulation->Get<Life>(m_Parent);
        life.Health(PETAL_DATA[m_Id].m_BaseHealth * PETAL_HEALTH_FACTOR[v]);
        life.MaxHealth(PETAL_DATA[m_Id].m_BaseHealth * PETAL_HEALTH_FACTOR[v]);
        life.m_Damage = PETAL_DATA[m_Id].m_BaseDamage * PETAL_DAMAGE_FACTOR[v];
        if (v == m_Rarity)
            return;
        m_Rarity = v;
        m_State |= 2;
    }

    void Petal::Shootable(bool v)
    {
        if (v == m_Shootable)
            return;
        m_Shootable = v;
        m_State |= 4;
    }

    void Petal::Independent(bool v)
    {
        if (v == m_Independent)
            return;
        m_Independent = v;
        m_State |= 8;
    }
}
