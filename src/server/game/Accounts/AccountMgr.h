/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#ifndef SF_ACCMGR_H
#define SF_ACCMGR_H

#include "Common.h"
#include "RBAC.h"
#include <ace/Singleton.h>

enum class AccountOpResult
{
    AOR_OK,
    AOR_NAME_TOO_LONG,
    AOR_PASS_TOO_LONG,
    AOR_EMAIL_TOO_LONG,
    AOR_NAME_ALREADY_EXIST,
    AOR_NAME_NOT_EXIST,
    AOR_DB_INTERNAL_ERROR
};

enum PasswordChangeSecurity
{
    PW_NONE,
    PW_EMAIL,
    PW_RBAC
};

#define MAX_ACCOUNT_STR 16
#define MAX_EMAIL_STR 64

namespace rbac
{
    typedef std::map<uint32, rbac::RBACPermission*> RBACPermissionsContainer;
    typedef std::map<uint8, rbac::RBACPermissionContainer> RBACDefaultPermissionsContainer;
}

class AccountMgr
{
    friend class ACE_Singleton<AccountMgr, ACE_Null_Mutex>;

private:
    AccountMgr();
    ~AccountMgr();

public:
    AccountOpResult CreateAccount(std::string username, std::string password, std::string email);
    static AccountOpResult DeleteAccount(uint32 accountId);
    static AccountOpResult ChangeUsername(uint32 accountId, std::string newUsername, std::string newPassword);
    static AccountOpResult ChangePassword(uint32 accountId, std::string newPassword);
    static AccountOpResult ChangeEmail(uint32 accountId, std::string newEmail);
    static AccountOpResult ChangeRegEmail(uint32 accountId, std::string newEmail);
    static bool CheckPassword(uint32 accountId, std::string password);
    static bool CheckEmail(uint32 accountId, std::string newEmail);

    static uint32 GetId(std::string const& username);
    static AccountTypes GetSecurity(uint32 accountId);
    static AccountTypes GetSecurity(uint32 accountId, int32 realmId);
    static bool GetName(uint32 accountId, std::string& name);
    static bool GetEmail(uint32 accountId, std::string& email);
    static uint32 GetCharactersCount(uint32 accountId);

    static bool normalizeString(std::string& utf8String);
    static bool IsPlayerAccount(AccountTypes gmlevel);
    static bool IsAdminAccount(AccountTypes gmlevel);
    static bool IsConsoleAccount(AccountTypes gmlevel);
    static bool HasPermission(uint32 accountId, uint32 permission, uint32 realmId);

    void UpdateAccountAccess(rbac::RBACData* rbac, uint32 accountId, uint8 securityLevel, int32 realmId);

    void LoadRBAC();
    rbac::RBACPermission const* GetRBACPermission(uint32 permission) const;

    rbac::RBACPermissionsContainer const& GetRBACPermissionList() const { return _permissions; }
    rbac::RBACPermissionContainer const& GetRBACDefaultPermissions(uint8 secLevel);

private:
    void ClearRBAC();
    rbac::RBACPermissionsContainer _permissions;
    rbac::RBACDefaultPermissionsContainer _defaultPermissions;
};

#define sAccountMgr ACE_Singleton<AccountMgr, ACE_Null_Mutex>::instance()
#endif
