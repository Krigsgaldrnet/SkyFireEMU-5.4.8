/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Boss_Dathrohan_Balnazzar
SD%Complete: 95
SDComment: Possibly need to fix/improve summons after death
SDCategory: Stratholme
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum Spells
{
    //Dathrohan spells
    SPELL_CRUSADERSHAMMER           = 17286,                //AOE stun
    SPELL_CRUSADERSTRIKE            = 17281,
    SPELL_HOLYSTRIKE                = 17284,                //weapon dmg +3

    //Transform
    SPELL_BALNAZZARTRANSFORM        = 17288,                //restore full HP/mana, trigger spell Balnazzar Transform Stun

    //Balnazzar spells
    SPELL_SHADOWSHOCK               = 17399,
    SPELL_MINDBLAST                 = 17287,
    SPELL_PSYCHICSCREAM             = 13704,
    SPELL_SLEEP                     = 12098,
    SPELL_MINDCONTROL               = 15690
};

enum Creatures
{
    NPC_DATHROHAN                   = 10812,
    NPC_BALNAZZAR                   = 10813,
    NPC_ZOMBIE                      = 10698                 //probably incorrect
};

struct SummonDef
{
    float m_fX, m_fY, m_fZ, m_fOrient;
};

SummonDef m_aSummonPoint[]=
{
    {3444.156f, -3090.626f, 135.002f, 2.240f},                  //G1 front, left
    {3449.123f, -3087.009f, 135.002f, 2.240f},                  //G1 front, right
    {3446.246f, -3093.466f, 135.002f, 2.240f},                  //G1 back left
    {3451.160f, -3089.904f, 135.002f, 2.240f},                  //G1 back, right

    {3457.995f, -3080.916f, 135.002f, 3.784f},                  //G2 front, left
    {3454.302f, -3076.330f, 135.002f, 3.784f},                  //G2 front, right
    {3460.975f, -3078.901f, 135.002f, 3.784f},                  //G2 back left
    {3457.338f, -3073.979f, 135.002f, 3.784f}                   //G2 back, right
};

class boss_dathrohan_balnazzar : public CreatureScript
{
public:
    boss_dathrohan_balnazzar() : CreatureScript("boss_dathrohan_balnazzar") { }

    CreatureAI* GetAI(Creature* creature) const OVERRIDE
    {
        return new boss_dathrohan_balnazzarAI(creature);
    }

    struct boss_dathrohan_balnazzarAI : public ScriptedAI
    {
        boss_dathrohan_balnazzarAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 m_uiCrusadersHammer_Timer;
        uint32 m_uiCrusaderStrike_Timer;
        uint32 m_uiMindBlast_Timer;
        uint32 m_uiHolyStrike_Timer;
        uint32 m_uiShadowShock_Timer;
        uint32 m_uiPsychicScream_Timer;
        uint32 m_uiDeepSleep_Timer;
        uint32 m_uiMindControl_Timer;
        bool m_bTransformed;

        void Reset() OVERRIDE
        {
            m_uiCrusadersHammer_Timer = 8000;
            m_uiCrusaderStrike_Timer = 12000;
            m_uiMindBlast_Timer = 6000;
            m_uiHolyStrike_Timer = 18000;
            m_uiShadowShock_Timer = 4000;
            m_uiPsychicScream_Timer = 16000;
            m_uiDeepSleep_Timer = 20000;
            m_uiMindControl_Timer = 10000;
            m_bTransformed = false;

            if (me->GetEntry() == NPC_BALNAZZAR)
                me->UpdateEntry(NPC_DATHROHAN);
        }

        void JustDied(Unit* /*killer*/) OVERRIDE
        {
            static uint32 uiCount = sizeof(m_aSummonPoint)/sizeof(SummonDef);

            for (uint8 i=0; i<uiCount; ++i)
                me->SummonCreature(NPC_ZOMBIE,
                m_aSummonPoint[i].m_fX, m_aSummonPoint[i].m_fY, m_aSummonPoint[i].m_fZ, m_aSummonPoint[i].m_fOrient,
                    TempSummonType::TEMPSUMMON_TIMED_DESPAWN, HOUR*IN_MILLISECONDS);
        }

        void EnterCombat(Unit* /*who*/) OVERRIDE
        {
        }

        void UpdateAI(uint32 uiDiff) OVERRIDE
        {
            if (!UpdateVictim())
                return;

            //START NOT TRANSFORMED
            if (!m_bTransformed)
            {
                //MindBlast
                if (m_uiMindBlast_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_MINDBLAST);
                    m_uiMindBlast_Timer = std::rand() % 20000 + 15000;
                } else m_uiMindBlast_Timer -= uiDiff;

                //CrusadersHammer
                if (m_uiCrusadersHammer_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_CRUSADERSHAMMER);
                    m_uiCrusadersHammer_Timer = 12000;
                } else m_uiCrusadersHammer_Timer -= uiDiff;

                //CrusaderStrike
                if (m_uiCrusaderStrike_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_CRUSADERSTRIKE);
                    m_uiCrusaderStrike_Timer = 15000;
                } else m_uiCrusaderStrike_Timer -= uiDiff;

                //HolyStrike
                if (m_uiHolyStrike_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_HOLYSTRIKE);
                    m_uiHolyStrike_Timer = 15000;
                } else m_uiHolyStrike_Timer -= uiDiff;

                //BalnazzarTransform
                if (HealthBelowPct(40))
                {
                    if (me->IsNonMeleeSpellCasted(false))
                        me->InterruptNonMeleeSpells(false);

                    //restore hp, mana and stun
                    DoCast(me, SPELL_BALNAZZARTRANSFORM);
                    me->UpdateEntry(NPC_BALNAZZAR);
                    m_bTransformed = true;
                }
            }
            else
            {
                //MindBlast
                if (m_uiMindBlast_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_MINDBLAST);
                    m_uiMindBlast_Timer = std::rand() % 20000 + 15000;
                } else m_uiMindBlast_Timer -= uiDiff;

                //ShadowShock
                if (m_uiShadowShock_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_SHADOWSHOCK);
                    m_uiShadowShock_Timer = 11000;
                } else m_uiShadowShock_Timer -= uiDiff;

                //PsychicScream
                if (m_uiPsychicScream_Timer <= uiDiff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_PSYCHICSCREAM);

                    m_uiPsychicScream_Timer = 20000;
                } else m_uiPsychicScream_Timer -= uiDiff;

                //DeepSleep
                if (m_uiDeepSleep_Timer <= uiDiff)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                        DoCast(target, SPELL_SLEEP);

                    m_uiDeepSleep_Timer = 15000;
                } else m_uiDeepSleep_Timer -= uiDiff;

                //MindControl
                if (m_uiMindControl_Timer <= uiDiff)
                {
                    DoCastVictim(SPELL_MINDCONTROL);
                    m_uiMindControl_Timer = 15000;
                } else m_uiMindControl_Timer -= uiDiff;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_dathrohan_balnazzar()
{
    new boss_dathrohan_balnazzar();
}
