/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Yells
{
    YELL_AGGRO                                    = 0,
    YELL_EVADE                                    = 1,
  //YELL_RESPAWN1                                 = -1810010, // Missing in database
  //YELL_RESPAWN2                                 = -1810011, // Missing in database
    YELL_RANDOM                                   = 2,
    YELL_SPELL                                    = 3,
};

enum Spells
{
    SPELL_AVATAR                                  = 19135,
    SPELL_THUNDERCLAP                             = 15588,
    SPELL_STORMBOLT                               = 20685 // not sure
};

class boss_vanndar : public CreatureScript
{
public:
    boss_vanndar() : CreatureScript("boss_vanndar") { }

    struct boss_vanndarAI : public ScriptedAI
    {
        boss_vanndarAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 AvatarTimer;
        uint32 ThunderclapTimer;
        uint32 StormboltTimer;
        uint32 ResetTimer;
        uint32 YellTimer;

        void Reset() OVERRIDE
        {
            AvatarTimer        = 3 * IN_MILLISECONDS;
            ThunderclapTimer   = 4 * IN_MILLISECONDS;
            StormboltTimer     = 6 * IN_MILLISECONDS;
            ResetTimer         = 5 * IN_MILLISECONDS;
            YellTimer = std::rand() % (30 * IN_MILLISECONDS) + (20 * IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
            Talk(YELL_AGGRO);
        }

        void UpdateAI(uint32 diff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            if (AvatarTimer <= diff)
            {
                DoCastVictim(SPELL_AVATAR);
                AvatarTimer =  std::rand() % (20 * IN_MILLISECONDS) + (15 * IN_MILLISECONDS);
            } else AvatarTimer -= diff;

            if (ThunderclapTimer <= diff)
            {
                DoCastVictim(SPELL_THUNDERCLAP);
                ThunderclapTimer = std::rand() % (15 * IN_MILLISECONDS) + (5 * IN_MILLISECONDS);
            } else ThunderclapTimer -= diff;

            if (StormboltTimer <= diff)
            {
                DoCastVictim(SPELL_STORMBOLT);
                StormboltTimer = std::rand() % (25 * IN_MILLISECONDS) + (10 * IN_MILLISECONDS);
            } else StormboltTimer -= diff;

            if (YellTimer <= diff)
            {
                Talk(YELL_RANDOM);
                YellTimer = std::rand() % (30 * IN_MILLISECONDS) + (20 * IN_MILLISECONDS); //20 to 30 seconds
            } else YellTimer -= diff;

            // check if creature is not outside of building
            if (ResetTimer <= diff)
            {
                if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 50)
                {
                    EnterEvadeMode();
                    Talk(YELL_EVADE);
                }
                ResetTimer = 5 * IN_MILLISECONDS;
            } else ResetTimer -= diff;

            DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_vanndarAI(creature);
    }
};

void AddSC_boss_vanndar()
{
    new boss_vanndar;
}