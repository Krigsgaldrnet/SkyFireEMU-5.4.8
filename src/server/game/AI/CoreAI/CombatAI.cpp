/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "CombatAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Vehicle.h"

int AggressorAI::Permissible(const Creature* creature)
{
    // have some hostile factions, it will be selected by IsHostileTo check at MoveInLineOfSight
    if (!creature->IsCivilian() && !creature->IsNeutralToAll())
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

void AggressorAI::UpdateAI(uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    DoMeleeAttackIfReady();
}

// some day we will delete these useless things
int CombatAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int ArcherAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int TurretAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

int VehicleAI::Permissible(const Creature* /*creature*/)
{
    return PERMIT_BASE_NO;
}

void CombatAI::InitializeAI()
{
    for (uint32 i = 0; i < CREATURE_MAX_SPELLS; ++i)
        if (me->m_spells[i] && sSpellMgr->GetSpellInfo(me->m_spells[i]))
            spells.push_back(me->m_spells[i]);

    CreatureAI::InitializeAI();
}

void CombatAI::Reset()
{
    events.Reset();
}

void CombatAI::JustDied(Unit* killer)
{
    for (SpellVct::iterator i = spells.begin(); i != spells.end(); ++i)
        if (AISpellInfo[*i].condition == AICOND_DIE)
            me->CastSpell(killer, *i, true);
}

void CombatAI::EnterCombat(Unit* who)
{
    for (SpellVct::iterator i = spells.begin(); i != spells.end(); ++i)
    {
        if (AISpellInfo[*i].condition == AICOND_AGGRO)
            me->CastSpell(who, *i, false);
        else if (AISpellInfo[*i].condition == AICOND_COMBAT)
            events.ScheduleEvent(*i, AISpellInfo[*i].cooldown + rand() % AISpellInfo[*i].cooldown);
    }
}

void CombatAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    if (uint32 spellId = events.ExecuteEvent())
    {
        DoCast(spellId);
        events.ScheduleEvent(spellId, AISpellInfo[spellId].cooldown + rand() % AISpellInfo[spellId].cooldown);
    }
    else
        DoMeleeAttackIfReady();
}

void CombatAI::SpellInterrupted(uint32 spellId, uint32 unTimeMs)
{
    events.RescheduleEvent(spellId, unTimeMs);
}

/////////////////
//CasterAI
/////////////////

void CasterAI::InitializeAI()
{
    CombatAI::InitializeAI();

    m_attackDist = 30.0f;
    for (SpellVct::iterator itr = spells.begin(); itr != spells.end(); ++itr)
        if (AISpellInfo[*itr].condition == AICOND_COMBAT && m_attackDist > GetAISpellInfo(*itr)->maxRange)
            m_attackDist = GetAISpellInfo(*itr)->maxRange;
    if (m_attackDist == 30.0f)
        m_attackDist = MELEE_RANGE;
}

void CasterAI::EnterCombat(Unit* who)
{
    if (spells.empty())
        return;

    uint32 spell = rand() % spells.size();
    uint32 count = 0;
    for (SpellVct::iterator itr = spells.begin(); itr != spells.end(); ++itr, ++count)
    {
        if (AISpellInfo[*itr].condition == AICOND_AGGRO)
            me->CastSpell(who, *itr, false);
        else if (AISpellInfo[*itr].condition == AICOND_COMBAT)
        {
            uint32 cooldown = GetAISpellInfo(*itr)->realCooldown;
            if (count == spell)
            {
                DoCast(spells[spell]);
                cooldown += me->GetCurrentSpellCastTime(*itr);
            }
            events.ScheduleEvent(*itr, cooldown);
        }
    }
}

void CasterAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->GetVictim()->HasBreakableByDamageCrowdControlAura(me))
    {
        me->InterruptNonMeleeSpells(false);
        return;
    }

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    if (uint32 spellId = events.ExecuteEvent())
    {
        DoCast(spellId);
        uint32 casttime = me->GetCurrentSpellCastTime(spellId);
        events.ScheduleEvent(spellId, (casttime ? casttime : 500) + GetAISpellInfo(spellId)->realCooldown);
    }
}

//////////////
//ArcherAI
//////////////

ArcherAI::ArcherAI(Creature* c) : CreatureAI(c)
{
    if (!me->m_spells[0])
        SF_LOG_ERROR("misc", "ArcherAI set for creature (entry = %u) with spell1=0. AI will do nothing", me->GetEntry());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(me->m_spells[0]);
    m_minRange = spellInfo ? spellInfo->GetMinRange(false) : 0;

    if (!m_minRange)
        m_minRange = MELEE_RANGE;
    me->m_CombatDistance = spellInfo ? spellInfo->GetMaxRange(false) : 0;
    me->m_SightDistance = me->m_CombatDistance;
}

void ArcherAI::AttackStart(Unit* who)
{
    if (!who)
        return;

    if (me->IsWithinCombatRange(who, m_minRange))
    {
        if (me->Attack(who, true) && !who->IsFlying())
            me->GetMotionMaster()->MoveChase(who);
    }
    else
    {
        if (me->Attack(who, false) && !who->IsFlying())
            me->GetMotionMaster()->MoveChase(who, me->m_CombatDistance);
    }

    if (who->IsFlying())
        me->GetMotionMaster()->MoveIdle();
}

void ArcherAI::UpdateAI(uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    if (!me->IsWithinCombatRange(me->GetVictim(), m_minRange))
        DoSpellAttackIfReady(me->m_spells[0]);
    else
        DoMeleeAttackIfReady();
}

//////////////
//TurretAI
//////////////

TurretAI::TurretAI(Creature* c) : CreatureAI(c)
{
    if (!me->m_spells[0])
        SF_LOG_ERROR("misc", "TurretAI set for creature (entry = %u) with spell1=0. AI will do nothing", me->GetEntry());

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(me->m_spells[0]);
    m_minRange = spellInfo ? spellInfo->GetMinRange(false) : 0;
    me->m_CombatDistance = spellInfo ? spellInfo->GetMaxRange(false) : 0;
    me->m_SightDistance = me->m_CombatDistance;
}

bool TurretAI::CanAIAttack(const Unit* /*who*/) const
{
    /// @todo use one function to replace it
    if (!me->IsWithinCombatRange(me->GetVictim(), me->m_CombatDistance)
        || (m_minRange && me->IsWithinCombatRange(me->GetVictim(), m_minRange)))
        return false;
    return true;
}

void TurretAI::AttackStart(Unit* who)
{
    if (who)
        me->Attack(who, false);
}

void TurretAI::UpdateAI(uint32 /*diff*/)
{
    if (!UpdateVictim())
        return;

    DoSpellAttackIfReady(me->m_spells[0]);
}

//////////////
//VehicleAI
//////////////

VehicleAI::VehicleAI(Creature* c) : CreatureAI(c), m_IsVehicleInUse(false), m_ConditionsTimer(VEHICLE_CONDITION_CHECK_TIME)
{
    LoadConditions();
    m_DoDismiss = false;
    m_DismissTimer = VEHICLE_DISMISS_TIME;
}

//NOTE: VehicleAI::UpdateAI runs even while the vehicle is mounted
void VehicleAI::UpdateAI(uint32 diff)
{
    CheckConditions(diff);

    if (m_DoDismiss)
    {
        if (m_DismissTimer < diff)
        {
            m_DoDismiss = false;
            me->SetVisible(false);
            me->DespawnOrUnsummon();
        }
        else
            m_DismissTimer -= diff;
    }
}

void VehicleAI::Reset()
{
    me->SetVisible(true);
}

void VehicleAI::OnCharmed(bool apply)
{
    if (m_IsVehicleInUse && !apply && !conditions.empty())//was used and has conditions
    {
        m_DoDismiss = true;//needs reset
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_PLAYER_VEHICLE);
        me->RemoveFlag(UNIT_FIELD_NPC_FLAGS, UNIT_NPC_FLAG_SPELLCLICK);
    }
    else if (apply)
        m_DoDismiss = false;//in use again

    m_DismissTimer = VEHICLE_DISMISS_TIME;//reset timer
    m_IsVehicleInUse = apply;
}

void VehicleAI::LoadConditions()
{
    conditions = sConditionMgr->GetConditionsForNotGroupedEntry(CONDITION_SOURCE_TYPE_CREATURE_TEMPLATE_VEHICLE, me->GetEntry());
    if (!conditions.empty())
        SF_LOG_DEBUG("condition", "VehicleAI::LoadConditions: loaded %u conditions", uint32(conditions.size()));
}

void VehicleAI::CheckConditions(const uint32 diff)
{
    if (m_ConditionsTimer < diff)
    {
        if (!conditions.empty())
        {
            if (Vehicle* vehicleKit = me->GetVehicleKit())
                for (SeatMap::iterator itr = vehicleKit->Seats.begin(); itr != vehicleKit->Seats.end(); ++itr)
                    if (Unit* passenger = ObjectAccessor::GetUnit(*me, itr->second.Passenger.Guid))
                    {
                        if (Player* player = passenger->ToPlayer())
                        {
                            if (!sConditionMgr->IsObjectMeetToConditions(player, me, conditions))
                            {
                                player->ExitVehicle();
                                return;//check other pessanger in next tick
                            }
                        }
                    }
        }
        m_ConditionsTimer = VEHICLE_CONDITION_CHECK_TIME;
    }
    else
        m_ConditionsTimer -= diff;
}
