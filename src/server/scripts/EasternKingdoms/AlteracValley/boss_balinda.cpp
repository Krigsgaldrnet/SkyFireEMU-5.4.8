/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    SPELL_ARCANE_EXPLOSION                        = 46608,
    SPELL_CONE_OF_COLD                            = 38384,
    SPELL_FIREBALL                                = 46988,
    SPELL_FROSTBOLT                               = 46987
};

enum Yells
{
    YELL_AGGRO                                   = 0,
    YELL_EVADE                                   = 1,
    YELL_SALVATION                               = 2,
};

enum Creatures
{
    NPC_WATER_ELEMENTAL                           = 25040
};

enum WaterElementalSpells
{
    SPELL_WATERBOLT                               = 46983
};

class npc_water_elemental : public CreatureScript
{
public:
    npc_water_elemental() : CreatureScript("npc_water_elemental") { }

    struct npc_water_elementalAI : public ScriptedAI
    {
        npc_water_elementalAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 waterBoltTimer;
        uint64 balindaGUID;
        uint32 resetTimer;

        void Reset() OVERRIDE
        {
            waterBoltTimer            = 3 * IN_MILLISECONDS;
            resetTimer                = 5 * IN_MILLISECONDS;
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (waterBoltTimer < diff)
            {
                DoCastVictim(SPELL_WATERBOLT);
                waterBoltTimer = 5 * IN_MILLISECONDS;
            } else waterBoltTimer -= diff;

            // check if creature is not outside of building
            if (resetTimer < diff)
            {
                if (Creature* pBalinda = Unit::GetCreature(*me, balindaGUID))
                    if (me->GetDistance2d(pBalinda->GetHomePosition().GetPositionX(), pBalinda->GetHomePosition().GetPositionY()) > 50)
                        EnterEvadeMode();
                resetTimer = 5 * IN_MILLISECONDS;
            } else resetTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new npc_water_elementalAI(creature);
    }
};

class boss_balinda : public CreatureScript
{
public:
    boss_balinda() : CreatureScript("boss_balinda") { }

    struct boss_balindaAI : public ScriptedAI
    {
        boss_balindaAI(Creature* creature) : ScriptedAI(creature), summons(me) { }

        uint32 arcaneExplosionTimer;
        uint32 coneOfColdTimer;
        uint32 fireBoltTimer;
        uint32 frostboltTimer;
        uint32 resetTimer;
        uint32 waterElementalTimer;

        SummonList summons;

        void Reset() OVERRIDE
        {
            arcaneExplosionTimer      = std::rand() % (15 * IN_MILLISECONDS) + (5 * IN_MILLISECONDS);
            coneOfColdTimer           = 8 * IN_MILLISECONDS;
            fireBoltTimer             = 1 * IN_MILLISECONDS;
            frostboltTimer            = 4 * IN_MILLISECONDS;
            resetTimer                = 5 * IN_MILLISECONDS;
            waterElementalTimer       = 0;

            summons.DespawnAll();
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(YELL_AGGRO);
        }

        void JustRespawned() OVERRIDE
        {
            Reset();
        }

        void JustSummoned(Creature* summoned) OVERRIDE
        {
            CAST_AI(npc_water_elemental::npc_water_elementalAI, summoned->AI())->balindaGUID = me->GetGUID();
            summoned->AI()->AttackStart(SelectTarget(SELECT_TARGET_RANDOM, 0, 50, true));
            summoned->setFaction(me->getFaction());
            summons.Summon(summoned);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            summons.DespawnAll();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (waterElementalTimer < diff)
            {
                if (summons.empty())
                    me->SummonCreature(NPC_WATER_ELEMENTAL, 0, 0, 0, 0, TempSummonType::TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45 * IN_MILLISECONDS);
                waterElementalTimer = 50 * IN_MILLISECONDS;
            } else waterElementalTimer -= diff;

            if (arcaneExplosionTimer < diff)
            {
                DoCastVictim(SPELL_ARCANE_EXPLOSION);
                arcaneExplosionTimer =  std::rand() % (15 * IN_MILLISECONDS) + (5 * IN_MILLISECONDS);
            } else arcaneExplosionTimer -= diff;

            if (coneOfColdTimer < diff)
            {
                DoCastVictim(SPELL_CONE_OF_COLD);
                coneOfColdTimer = std::rand() % (20 * IN_MILLISECONDS) + (10 * IN_MILLISECONDS);
            } else coneOfColdTimer -= diff;

            if (fireBoltTimer < diff)
            {
                DoCastVictim(SPELL_FIREBALL);
                fireBoltTimer = std::rand() % (9 * IN_MILLISECONDS) + (5 * IN_MILLISECONDS);
            } else fireBoltTimer -= diff;

            if (frostboltTimer < diff)
            {
                DoCastVictim(SPELL_FROSTBOLT);
                frostboltTimer = std::rand() % (12 * IN_MILLISECONDS) + (4 * IN_MILLISECONDS);
            } else frostboltTimer -= diff;

            // check if creature is not outside of building
            if (resetTimer < diff)
            {
                if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 50)
                {
                    EnterEvadeMode();
                    Talk(YELL_EVADE);
                }
                resetTimer = 5 * IN_MILLISECONDS;
            } else resetTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_balindaAI(creature);
    }
};

void AddSC_boss_balinda()
{
    new boss_balinda;
    new npc_water_elemental;
};
