/*
* This file is part of Project SkyFire https://www.projectskyfire.org. 
* See LICENSE.md file for Copyright information
*/

/* ScriptData
SDName: Npc_Innkeeper
SDAuthor: WarHead
SD%Complete: 99%
SDComment: Complete
SDCategory: NPCs
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "GameEventMgr.h"
#include "Player.h"
#include "WorldSession.h"

enum Spells
{
    SPELL_TRICK_OR_TREATED      = 24755,
    SPELL_TREAT                 = 24715
};

#define LOCALE_TRICK_OR_TREAT_0 "Trick or Treat!"
#define LOCALE_TRICK_OR_TREAT_2 "Des bonbons ou des blagues!"
#define LOCALE_TRICK_OR_TREAT_3 "Süßes oder Saures!"
#define LOCALE_TRICK_OR_TREAT_6 "¡Truco o trato!"

#define LOCALE_INNKEEPER_0 "Make this inn my home."
#define LOCALE_INNKEEPER_3 "Ich möchte dieses Gasthaus zu meinem Heimatort machen."

class npc_innkeeper : public CreatureScript
{
public:
    npc_innkeeper() : CreatureScript("npc_innkeeper") { }

    bool OnGossipHello(Player* player, Creature* creature) OVERRIDE
    {
        if (IsHolidayActive(HolidayIds::HOLIDAY_HALLOWS_END) && !player->HasAura(SPELL_TRICK_OR_TREATED))
        {
            const char* localizedEntry;
            switch (player->GetSession()->GetSessionDbcLocale())
            {
                case LOCALE_frFR: localizedEntry = LOCALE_TRICK_OR_TREAT_2; break;
                case LOCALE_deDE: localizedEntry = LOCALE_TRICK_OR_TREAT_3; break;
                case LOCALE_esES: localizedEntry = LOCALE_TRICK_OR_TREAT_6; break;
                case LOCALE_enUS: default: localizedEntry = LOCALE_TRICK_OR_TREAT_0;
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, localizedEntry, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        }

        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor())
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        if (creature->IsInnkeeper())
        {
            const char* localizedEntry;
            switch (player->GetSession()->GetSessionDbcLocale())
            {
                case LOCALE_deDE: localizedEntry = LOCALE_INNKEEPER_3; break;
                case LOCALE_enUS: default: localizedEntry = LOCALE_INNKEEPER_0;
            }
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_INTERACT_1, localizedEntry, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INN);
        }

        player->TalkedToCreature(creature->GetEntry(), creature->GetGUID());
        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) OVERRIDE
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_INFO_DEF + 1 && IsHolidayActive(HolidayIds::HOLIDAY_HALLOWS_END) && !player->HasAura(SPELL_TRICK_OR_TREATED))
        {
            player->CastSpell(player, SPELL_TRICK_OR_TREATED, true);

            if (std::rand() % 1)
                player->CastSpell(player, SPELL_TREAT, true);
            else
            {
                uint32 trickspell = 0;
                switch (std::rand() % 13)
                {
                    case 0: trickspell = 24753; break; // cannot cast, random 30sec
                    case 1: trickspell = 24713; break; // lepper gnome costume
                    case 2: trickspell = 24735; break; // male ghost costume
                    case 3: trickspell = 24736; break; // female ghostcostume
                    case 4: trickspell = 24710; break; // male ninja costume
                    case 5: trickspell = 24711; break; // female ninja costume
                    case 6: trickspell = 24708; break; // male pirate costume
                    case 7: trickspell = 24709; break; // female pirate costume
                    case 8: trickspell = 24723; break; // skeleton costume
                    case 9: trickspell = 24753; break; // Trick
                    case 10: trickspell = 24924; break; // Hallow's End Candy
                    case 11: trickspell = 24925; break; // Hallow's End Candy
                    case 12: trickspell = 24926; break; // Hallow's End Candy
                    case 13: trickspell = 24927; break; // Hallow's End Candy
                }
                player->CastSpell(player, trickspell, true);
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }

        player->CLOSE_GOSSIP_MENU();

        switch (action)
        {
            case GOSSIP_ACTION_TRADE: player->GetSession()->SendListInventory(creature->GetGUID()); break;
            case GOSSIP_ACTION_INN: player->SetBindPoint(creature->GetGUID()); break;
        }
        return true;
    }
};

void AddSC_npc_innkeeper()
{
    new npc_innkeeper;
}

