/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

/* ScriptData
Name: learn_commandscript
%Complete: 100
Comment: All learn related commands
Category: commandscripts
EndScriptData */

#include "Chat.h"
#include "Language.h"
#include "ObjectMgr.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

class learn_commandscript : public CommandScript
{
public:
    learn_commandscript() : CommandScript("learn_commandscript") { }

    std::vector<ChatCommand> GetCommands() const OVERRIDE
    {
        static std::vector<ChatCommand> learnAllMyCommandTable =
        {
            { "class",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_CLASS,      false, &HandleLearnAllMyClassCommand,      "", },
            { "spells",     rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_SPELLS,     false, &HandleLearnAllMySpellsCommand,     "", },
            { "talents",    rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY_TALENTS,    false, &HandleLearnAllMyTalentsCommand,    "", },
        };

        static std::vector<ChatCommand> learnAllCommandTable =
        {
            { "my",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_MY,      false, NULL,                          "", learnAllMyCommandTable },
            { "gm",      rbac::RBAC_PERM_COMMAND_LEARN_ALL_GM,      false, &HandleLearnAllGMCommand,      "", },
            { "crafts",  rbac::RBAC_PERM_COMMAND_LEARN_ALL_CRAFTS,  false, &HandleLearnAllCraftsCommand,  "", },
            { "default", rbac::RBAC_PERM_COMMAND_LEARN_ALL_DEFAULT, false, &HandleLearnAllDefaultCommand, "", },
            { "lang",    rbac::RBAC_PERM_COMMAND_LEARN_ALL_LANG,    false, &HandleLearnAllLangCommand,    "", },
            { "recipes", rbac::RBAC_PERM_COMMAND_LEARN_ALL_RECIPES, false, &HandleLearnAllRecipesCommand, "", },
        };

        static std::vector<ChatCommand> learnCommandTable =
        {
            { "all", rbac::RBAC_PERM_COMMAND_LEARN_ALL, false, NULL,                "", learnAllCommandTable },
            { "",    rbac::RBAC_PERM_COMMAND_LEARN,     false, &HandleLearnCommand, "", },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "learn",   rbac::RBAC_PERM_COMMAND_LEARN,   false, NULL,                  "", learnCommandTable },
            { "unlearn", rbac::RBAC_PERM_COMMAND_UNLEARN, false, &HandleUnLearnCommand, "", },
        };
        return commandTable;
    }

    static bool HandleLearnCommand(ChatHandler* handler, char const* args)
    {
        Player* targetPlayer = handler->getSelectedPlayer();

        if (!targetPlayer)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r or Htalent form
        uint32 spell = handler->extractSpellIdFromLink((char*)args);
        if (!spell || !sSpellMgr->GetSpellInfo(spell))
            return false;

        char const* all = strtok(NULL, " ");
        bool allRanks = all ? (strncmp(all, "all", strlen(all)) == 0) : false;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell);
        if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer()))
        {
            handler->PSendSysMessage(LANG_COMMAND_SPELL_BROKEN, spell);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!allRanks && targetPlayer->HasSpell(spell))
        {
            if (targetPlayer == handler->GetSession()->GetPlayer())
                handler->SendSysMessage(LANG_YOU_KNOWN_SPELL);
            else
                handler->PSendSysMessage(LANG_TARGET_KNOWN_SPELL, handler->GetNameLink(targetPlayer).c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (allRanks)
            targetPlayer->learnSpellHighRank(spell);
        else
            targetPlayer->learnSpell(spell, false);

        if (GetTalentSpellCost(spellInfo->GetFirstRankSpell()->Id))
            targetPlayer->SendTalentsInfoData();

        return true;
    }

    static bool HandleLearnAllGMCommand(ChatHandler* handler, char const* /*args*/)
    {
        for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(i);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            if (!spellInfo->IsAbilityOfSkillType(SKILL_INTERNAL))
                continue;

            handler->GetSession()->GetPlayer()->learnSpell(i, false);
        }

        handler->SendSysMessage(LANG_LEARNING_GM_SKILLS);
        return true;
    }

    static bool HandleLearnAllMyClassCommand(ChatHandler* handler, char const* /*args*/)
    {
        HandleLearnAllMySpellsCommand(handler, "");
        HandleLearnAllMyTalentsCommand(handler, "");
        return true;
    }

    static bool HandleLearnAllMySpellsCommand(ChatHandler* handler, char const* /*args*/)
    {
        ChrClassesEntry const* classEntry = sChrClassesStore.LookupEntry(handler->GetSession()->GetPlayer()->getClass());
        if (!classEntry)
            return true;
        uint32 family = classEntry->spellfamily;

        for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
        {
            SkillLineAbilityEntry const* entry = sSkillLineAbilityStore.LookupEntry(i);
            if (!entry)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(entry->spellId);
            if (!spellInfo)
                continue;

            // skip server-side/triggered spells
            if (spellInfo->SpellLevel == 0)
                continue;

            // skip wrong class/race skills
            if (!handler->GetSession()->GetPlayer()->IsSpellFitByClassAndRace(spellInfo->Id))
                continue;

            // skip other spell families
            if (spellInfo->SpellFamilyName != family)
                continue;

            // skip spells with first rank learned as talent (and all talents then also)
            if (GetTalentSpellCost(spellInfo->GetFirstRankSpell()->Id) > 0)
                continue;

            // skip broken spells
            if (!SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            handler->GetSession()->GetPlayer()->learnSpell(spellInfo->Id, false);
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_SPELLS);
        return true;
    }

    static bool HandleLearnAllMyTalentsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();

        for (uint32 i = 0; i < sTalentStore.GetNumRows(); ++i)
        {
            TalentEntry const* talentInfo = sTalentStore.LookupEntry(i);
            if (!talentInfo)
                continue;

            if (talentInfo->playerClass != player->getClass())
                continue;

            // search highest talent rank
            uint32 spellId = talentInfo->SpellId;

            if (!spellId)                                        // ??? none spells in talent
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, handler->GetSession()->GetPlayer(), false))
                continue;

            // learn highest rank of talent and learn all non-talent spell ranks (recursive by tree)
            player->learnSpellHighRank(spellId);
            player->AddTalent(spellId, player->GetActiveSpec(), true);
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_CLASS_TALENTS);
        return true;
    }

    static bool HandleLearnAllLangCommand(ChatHandler* handler, char const* /*args*/)
    {
        // skipping UNIVERSAL language (0)
        for (uint8 i = 1; i < LANGUAGES_COUNT; ++i)
            handler->GetSession()->GetPlayer()->learnSpell(lang_description[i].spell_id, false);

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_LANG);
        return true;
    }

    static bool HandleLearnAllDefaultCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        target->learnDefaultSpells();
        target->learnQuestRewardedSpells();

        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_DEFAULT_AND_QUEST, handler->GetNameLink(target).c_str());
        return true;
    }

    static bool HandleLearnAllCraftsCommand(ChatHandler* handler, char const* args)
    {
        Player* target;
        if (!handler->extractPlayerTarget((char*)args, &target))
            return false;

        for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId == SKILL_CATEGORY_PROFESSION || skillInfo->categoryId == SKILL_CATEGORY_SECONDARY) &&
                skillInfo->canLink)                             // only prof. with recipes have
            {
                HandleLearnSkillRecipesHelper(target, skillInfo->id);
            }
        }

        handler->SendSysMessage(LANG_COMMAND_LEARN_ALL_CRAFT);
        return true;
    }

    static bool HandleLearnAllRecipesCommand(ChatHandler* handler, char const* args)
    {
        //  Learns all recipes of specified profession and sets skill to max
        //  Example: .learn all_recipes enchanting

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            return false;
        }

        if (!*args)
            return false;

        std::wstring namePart;

        if (!Utf8toWStr(args, namePart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(namePart);

        std::string name;

        SkillLineEntry const* targetSkillInfo = NULL;
        for (uint32 i = 1; i < sSkillLineStore.GetNumRows(); ++i)
        {
            SkillLineEntry const* skillInfo = sSkillLineStore.LookupEntry(i);
            if (!skillInfo)
                continue;

            if ((skillInfo->categoryId != SKILL_CATEGORY_PROFESSION &&
                skillInfo->categoryId != SKILL_CATEGORY_SECONDARY) ||
                !skillInfo->canLink)                            // only prof with recipes have set
                continue;

            name = skillInfo->name;
            if (name.empty())
                continue;

            if (!Utf8FitTo(name, namePart))
                continue;

            targetSkillInfo = skillInfo;
        }

        if (!targetSkillInfo)
            return false;

        HandleLearnSkillRecipesHelper(target, targetSkillInfo->id);

        uint16 maxLevel = target->GetPureMaxSkillValue(targetSkillInfo->id);
        target->SetSkill(targetSkillInfo->id, target->GetSkillStep(targetSkillInfo->id), maxLevel, maxLevel);
        handler->PSendSysMessage(LANG_COMMAND_LEARN_ALL_RECIPES, name.c_str());
        return true;
    }

    static void HandleLearnSkillRecipesHelper(Player* player, uint32 skillId)
    {
        uint32 classmask = player->getClassMask();

        for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
        {
            SkillLineAbilityEntry const* skillLine = sSkillLineAbilityStore.LookupEntry(j);
            if (!skillLine)
                continue;

            // wrong skill
            if (skillLine->skillId != skillId)
                continue;

            // not high rank
            if (skillLine->forward_spellid)
                continue;

            // skip racial skills
            if (skillLine->racemask != 0)
                continue;

            // skip wrong class skills
            if (skillLine->classmask && (skillLine->classmask & classmask) == 0)
                continue;

            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(skillLine->spellId);
            if (!spellInfo || !SpellMgr::IsSpellValid(spellInfo, player, false))
                continue;

            player->learnSpell(skillLine->spellId, false);
        }
    }

    static bool HandleUnLearnCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
            return false;

        // number or [name] Shift-click form |color|Hspell:spell_id|h[name]|h|r
        uint32 spellId = handler->extractSpellIdFromLink((char*)args);
        if (!spellId)
            return false;

        char const* allStr = strtok(NULL, " ");
        bool allRanks = allStr ? (strncmp(allStr, "all", strlen(allStr)) == 0) : false;

        Player* target = handler->getSelectedPlayer();
        if (!target)
        {
            handler->SendSysMessage(LANG_NO_CHAR_SELECTED);
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (allRanks)
            spellId = sSpellMgr->GetFirstSpellInChain(spellId);

        if (target->HasSpell(spellId))
            target->removeSpell(spellId, false, !allRanks);
        else
            handler->SendSysMessage(LANG_FORGET_SPELL);

        if (GetTalentSpellCost(spellId))
            target->SendTalentsInfoData();

        return true;
    }
};

void AddSC_learn_commandscript()
{
    new learn_commandscript();
}
