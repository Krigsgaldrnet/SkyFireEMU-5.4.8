/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/
#ifndef SKYFIRESERVER_TEMPSUMMON_H
#define SKYFIRESERVER_TEMPSUMMON_H

#include "Creature.h"

enum class SummonerType
{
    SUMMONER_TYPE_CREATURE = 0,
    SUMMONER_TYPE_GAMEOBJECT = 1,
    SUMMONER_TYPE_MAP = 2
};

/// Stores data for temp summons
struct TempSummonData
{
    uint32 entry;        ///< Entry of summoned creature
    Position pos;        ///< Position, where should be creature spawned
    TempSummonType type; ///< Summon type, see TempSummonType for available types
    uint32 time;         ///< Despawn time, usable only with certain temp summon types
};

class TempSummon : public Creature
{
public:
    explicit TempSummon(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject);
    virtual ~TempSummon() { }
    void Update(uint32 time) OVERRIDE;
    virtual void InitStats(uint32 lifetime);
    virtual void InitSummon();
    virtual void UnSummon(uint32 msTime = 0);
    void RemoveFromWorld() OVERRIDE;
    void SetTempSummonType(TempSummonType type);
    void SaveToDB(uint32 /*mapid*/, uint32 /*spawnMask*/) OVERRIDE { }
    Unit* GetSummoner() const;
    Creature* GetSummonerCreatureBase() const;
    uint64 GetSummonerGUID() const { return m_summonerGUID; }
    TempSummonType const& GetSummonType() const { return m_type; }
    uint32 GetTimer() const { return m_timer; }

    const SummonPropertiesEntry* const m_Properties;
private:
    TempSummonType m_type;
    uint32 m_timer;
    uint32 m_lifetime;
    uint64 m_summonerGUID;
};

class Minion : public TempSummon
{
public:
    Minion(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject);
    void InitStats(uint32 duration) OVERRIDE;
    void RemoveFromWorld() OVERRIDE;
    Unit* GetOwner() const { return m_owner; }
    float GetFollowAngle() const OVERRIDE { return m_followAngle; }
    void SetFollowAngle(float angle) { m_followAngle = angle; }
    bool IsPetGhoul() const { return GetEntry() == 26125; } // Ghoul may be guardian or pet
    bool IsSpiritWolf() const { return GetEntry() == 29264; } // Spirit wolf from feral spirits
    bool IsWhiteTiger() const { return GetEntry() == 63508; } // Invoke Xuen
    bool IsGuardianPet() const;
protected:
    Unit* const m_owner;
    float m_followAngle;
};

class Guardian : public Minion
{
public:
    Guardian(SummonPropertiesEntry const* properties, Unit* owner, bool isWorldObject);
    void InitStats(uint32 duration) OVERRIDE;
    bool InitStatsForLevel(uint8 level);
    void InitSummon() OVERRIDE;

    bool UpdateStats(Stats stat) OVERRIDE;
    bool UpdateAllStats() OVERRIDE;
    void UpdateResistances(uint32 school) OVERRIDE;
    void UpdateArmor() OVERRIDE;
    void UpdateMaxHealth() OVERRIDE;
    void UpdateMaxPower(Powers power) OVERRIDE;
    void UpdateAttackPowerAndDamage(bool ranged = false) OVERRIDE;
    void UpdateDamagePhysical(WeaponAttackType attType) OVERRIDE;

    int32 GetBonusDamage() const { return m_bonusSpellDamage; }
    void SetBonusDamage(int32 damage);
protected:
    int32   m_bonusSpellDamage;
    float   m_statFromOwner[MAX_STATS];
};

class Puppet : public Minion
{
public:
    Puppet(SummonPropertiesEntry const* properties, Unit* owner);
    void InitStats(uint32 duration) OVERRIDE;
    void InitSummon() OVERRIDE;
    void Update(uint32 time) OVERRIDE;
    void RemoveFromWorld() OVERRIDE;
};

class ForcedUnsummonDelayEvent : public BasicEvent
{
public:
    ForcedUnsummonDelayEvent(TempSummon& owner) : BasicEvent(), m_owner(owner) { }
    bool Execute(uint64 e_time, uint32 p_time) OVERRIDE;

private:
    TempSummon& m_owner;
};
#endif
