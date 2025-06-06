/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/*
* Comment: No Waves atm and the doors spells are crazy...
*
* When your group enters the main room (the one after the bridge), you will notice a group of 3 Nerubians.
* When you engage them, 2 more groups like this one spawn behind the first one - it is important to pull the first group back,
* so you don't aggro all 3. Hadronox will be under you, fighting Nerubians.
*
* This is the timed gauntlet - waves of non-elite spiders
* will spawn from the 3 doors located a little above the main room, and will then head down to fight Hadronox. After clearing the
* main room, it is recommended to just stay in it, kill the occasional non-elites that will attack you instead of the boss, and wait for
* Hadronox to make his way to you. When Hadronox enters the main room, she will web the doors, and no more non-elites will spawn.
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "azjol_nerub.h"

enum Spells
{
    SPELL_ACID_CLOUD                              = 53400, // Victim
    SPELL_LEECH_POISON                            = 53030, // Victim
    SPELL_PIERCE_ARMOR                            = 53418, // Victim
    SPELL_WEB_GRAB                                = 57731, // Victim
    SPELL_WEB_FRONT_DOORS                         = 53177, // Self
    SPELL_WEB_SIDE_DOORS                          = 53185, // Self
    H_SPELL_ACID_CLOUD                            = 59419,
    H_SPELL_LEECH_POISON                          = 59417,
    H_SPELL_WEB_GRAB                              = 59421
};

class boss_hadronox : public CreatureScript
{
public:
    boss_hadronox() : CreatureScript("boss_hadronox") { }

    struct boss_hadronoxAI : public ScriptedAI
    {
        boss_hadronoxAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            fMaxDistance = 50.0f;
            bFirstTime = true;
        }

        InstanceScript* instance;

        uint32 uiAcidTimer;
        uint32 uiLeechTimer;
        uint32 uiPierceTimer;
        uint32 uiGrabTimer;
        uint32 uiDoorsTimer;
        uint32 uiCheckDistanceTimer;

        bool bFirstTime;

        float fMaxDistance;

        void Reset() OVERRIDE
        {
            me->SetFloatValue(UNIT_FIELD_BOUNDING_RADIUS, 9.0f);
            me->SetFloatValue(UNIT_FIELD_COMBAT_REACH, 9.0f);

            uiAcidTimer = std::rand() % (14 * IN_MILLISECONDS) + (10*IN_MILLISECONDS);
            uiLeechTimer = std::rand() % (9 * IN_MILLISECONDS) + (3*IN_MILLISECONDS);
            uiPierceTimer = std::rand() % (3 * IN_MILLISECONDS) + (1*IN_MILLISECONDS);
            uiGrabTimer = std::rand() % (19 * IN_MILLISECONDS) + (15*IN_MILLISECONDS);
            uiDoorsTimer = std::rand() % (30 * IN_MILLISECONDS) + (20*IN_MILLISECONDS);
            uiCheckDistanceTimer = 2*IN_MILLISECONDS;

            if (instance && (instance->GetBossState(DATA_HADRONOX) != DONE && !bFirstTime))
                instance->SetBossState(DATA_HADRONOX, FAIL);

            bFirstTime = false;
        }

        //when Hadronox kills any enemy (that includes a party member) she will regain 10% of her HP if the target had Leech Poison on
        void KilledUnit(Unit* Victim) OVERRIDE
        {
            // not sure if this aura check is correct, I think it is though
            if (!Victim || !Victim->HasAura(DUNGEON_MODE(SPELL_LEECH_POISON, H_SPELL_LEECH_POISON)) || !me->IsAlive())
                return;

            me->ModifyHealth(int32(me->CountPctFromMaxHealth(10)));
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            if (instance)
                instance->SetBossState(DATA_HADRONOX, DONE);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            if (instance)
                instance->SetBossState(DATA_HADRONOX, IN_PROGRESS);
            me->SetInCombatWithZone();
        }

        void CheckDistance(float dist, const uint32 uiDiff)
        {
            if (!me->IsInCombat())
                return;

            float x=0.0f, y=0.0f, z=0.0f;
            me->GetRespawnPosition(x, y, z);

            if (uiCheckDistanceTimer <= uiDiff)
                uiCheckDistanceTimer = 5*IN_MILLISECONDS;
            else
            {
                uiCheckDistanceTimer -= uiDiff;
                return;
            }
            if (me->IsInEvadeMode() || !me->GetVictim())
                return;
            if (me->GetDistance(x, y, z) > dist)
                EnterEvadeMode();
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            // Without he comes up through the air to players on the bridge after krikthir if players crossing this bridge!
            CheckDistance(fMaxDistance, diff);

            if (me->HasAura(SPELL_WEB_FRONT_DOORS) || me->HasAura(SPELL_WEB_SIDE_DOORS))
            {
                if (IsCombatMovementAllowed())
                    SetCombatMovement(false);
            }
            else if (!IsCombatMovementAllowed())
                SetCombatMovement(true);

            if (uiPierceTimer <= diff)
            {
                DoCastVictim(SPELL_PIERCE_ARMOR);
                uiPierceTimer = 8*IN_MILLISECONDS;
            } else uiPierceTimer -= diff;

            if (uiAcidTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_ACID_CLOUD);

                uiAcidTimer = std::rand() % (30 * IN_MILLISECONDS) + (20*IN_MILLISECONDS);
            } else uiAcidTimer -= diff;

            if (uiLeechTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100, true))
                    DoCast(target, SPELL_LEECH_POISON);

                uiLeechTimer = std::rand() % (14 * IN_MILLISECONDS) + (11*IN_MILLISECONDS);
            } else uiLeechTimer -= diff;

            if (uiGrabTimer <= diff)
            {
                if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0)) // Draws all players (and attacking Mobs) to itself.
                    DoCast(target, SPELL_WEB_GRAB);

                uiGrabTimer = std::rand() % (30 * IN_MILLISECONDS) + (15*IN_MILLISECONDS);
            } else uiGrabTimer -= diff;

            if (uiDoorsTimer <= diff)
            {
                uiDoorsTimer = std::rand() % (60 * IN_MILLISECONDS) + (30*IN_MILLISECONDS);
            } else uiDoorsTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_hadronoxAI(creature);
    }
};

void AddSC_boss_hadronox()
{
    new boss_hadronox();
}
