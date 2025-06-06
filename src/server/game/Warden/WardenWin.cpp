/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include "AccountMgr.h"
#include "ByteBuffer.h"
#include "Common.h"
#include "CryptoRandom.h"
#include "Database/DatabaseEnv.h"
#include "HMAC.h"
#include "Log.h"
#include "MD5.h"
#include "Opcodes.h"
#include "Player.h"
#include "SessionKeyGenerator.h"
#include "Util.h"
#include "WardenCheckMgr.h"
#include "WardenModuleWin.h"
#include "WardenWin.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

WardenWin::WardenWin() : Warden() { }

WardenWin::~WardenWin() { }

void WardenWin::Init(WorldSession* session, SessionKey const& k)
{
    _session = session;
    // Generate Warden Key
    SessionKeyGenerator<SkyFire::Crypto::SHA1> WK(k);
    WK.Generate(_inputKey, 16);
    WK.Generate(_outputKey, 16);

    memcpy(_seed, Module.Seed, 16);

    _inputCrypto.Init(_inputKey, 16);
    _outputCrypto.Init(_outputKey, 16);
    SF_LOG_DEBUG("warden", "Server side warden for client %u initializing...", session->GetAccountId());
    SF_LOG_DEBUG("warden", "C->S Key: %s", SkyFire::Impl::ByteArrayToHexStr(_inputKey, 16).c_str());
    SF_LOG_DEBUG("warden", "S->C Key: %s", SkyFire::Impl::ByteArrayToHexStr(_outputKey, 16).c_str());
    SF_LOG_DEBUG("warden", "  Seed: %s", SkyFire::Impl::ByteArrayToHexStr(_seed, 16).c_str());
    SF_LOG_DEBUG("warden", "Loading Module...");

    _module = GetModuleForClient();

    SF_LOG_DEBUG("warden", "Module Key: %s", SkyFire::Impl::ByteArrayToHexStr(_module->Key, 16).c_str());
    SF_LOG_DEBUG("warden", "Module ID: %s", SkyFire::Impl::ByteArrayToHexStr(_module->Id, 16).c_str());
    RequestModule();
}

ClientWardenModule* WardenWin::GetModuleForClient()
{
    ClientWardenModule* mod = new ClientWardenModule;

    uint32 length = sizeof(Module.Module);

    // data assign
    mod->CompressedSize = length;
    mod->CompressedData = new uint8[length];
    memcpy(mod->CompressedData, Module.Module, length);
    memcpy(mod->Key, Module.ModuleKey, 16);

    MD5Hash md5;
    md5.UpdateData(mod->CompressedData, length);
    md5.Finalize(mod->CompressedData, length);

    return mod;
}

void WardenWin::InitializeModule()
{
    SF_LOG_DEBUG("warden", "Initialize module");

    // Create packet structure
    WardenInitModuleRequest Request;
    Request.Command1 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size1 = 20;
    Request.Unk1 = 1;
    Request.Unk2 = 0;
    Request.Type = 1;
    Request.String_library1 = 0;
    Request.Function1[0] = 0x00024F80;                      // 0x00400000 + 0x00024F80 SFileOpenFile
    Request.Function1[1] = 0x000218C0;                      // 0x00400000 + 0x000218C0 SFileGetFileSize
    Request.Function1[2] = 0x00022530;                      // 0x00400000 + 0x00022530 SFileReadFile
    Request.Function1[3] = 0x00022910;                      // 0x00400000 + 0x00022910 SFileCloseFile
    Request.CheckSumm1 = BuildChecksum(&Request.Unk1, 20);

    Request.Command2 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size2 = 8;
    Request.Unk3 = 4;
    Request.Unk4 = 0;
    Request.String_library2 = 0;
    Request.Function2 = 0x00419D40;                         // 0x00400000 + 0x00419D40 FrameScript::GetText
    Request.Function2_set = 1;
    Request.CheckSumm2 = BuildChecksum(&Request.Unk2, 8);

    Request.Command3 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size3 = 8;
    Request.Unk5 = 1;
    Request.Unk6 = 1;
    Request.String_library3 = 0;
    Request.Function3 = 0x0046AE20;                         // 0x00400000 + 0x0046AE20 PerformanceCounter
    Request.Function3_set = 1;
    Request.CheckSumm3 = BuildChecksum(&Request.Unk5, 8);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenInitModuleRequest));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenInitModuleRequest));
    pkt.append((uint8*)&Request, sizeof(WardenInitModuleRequest));
    _session->SendPacket(&pkt);
}

void WardenWin::RequestHash()
{
    SF_LOG_DEBUG("warden", "Request hash");

    // Create packet structure
    WardenHashRequest Request;
    Request.Command = WARDEN_SMSG_HASH_REQUEST;
    memcpy(Request.Seed, _seed, 16);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenHashRequest));

    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenHashRequest));
    pkt.append((uint8*)&Request, sizeof(WardenHashRequest));
    _session->SendPacket(&pkt);
}

void WardenWin::HandleHashResult(ByteBuffer& buff)
{
    buff.rpos(buff.wpos());

    // Verify key
    if (memcmp(buff.contents() + 1, Module.ClientKeySeedHash, 20) != 0)
    {
        SF_LOG_WARN("warden", "%s failed hash reply. Action: %s", _session->GetPlayerInfo().c_str(), Penalty().c_str());
        return;
    }

    SF_LOG_DEBUG("warden", "Request hash reply: succeed");

    // Change keys here
    memcpy(_inputKey, Module.ClientKeySeed, 16);
    memcpy(_outputKey, Module.ServerKeySeed, 16);

    _inputCrypto.Init(_inputKey, 16);
    _outputCrypto.Init(_outputKey, 16);

    _initialized = true;

    _previousTimestamp = getMSTime();
}

void WardenWin::RequestData()
{
    SF_LOG_DEBUG("warden", "Request data");

    // If all checks were done, fill the todo list again
    if (_memChecksTodo.empty())
        _memChecksTodo.assign(sWardenCheckMgr->MemChecksIdPool.begin(), sWardenCheckMgr->MemChecksIdPool.end());

    if (_otherChecksTodo.empty())
        _otherChecksTodo.assign(sWardenCheckMgr->OtherChecksIdPool.begin(), sWardenCheckMgr->OtherChecksIdPool.end());

    _serverTicks = getMSTime();

    uint16 id;
    uint8 type;
    WardenCheck* wd;
    _currentChecks.clear();

    // Build check request
    for (uint32 i = 0; i < sWorld->getIntConfig(WorldIntConfigs::CONFIG_WARDEN_NUM_MEM_CHECKS); ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_memChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _memChecksTodo.back();
        _memChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);
    }

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);

    ACE_READ_GUARD(ACE_RW_Mutex, g, sWardenCheckMgr->_checkStoreLock);

    for (uint32 i = 0; i < sWorld->getIntConfig(WorldIntConfigs::CONFIG_WARDEN_NUM_OTHER_CHECKS); ++i)
    {
        // If todo list is done break loop (will be filled on next Update() run)
        if (_otherChecksTodo.empty())
            break;

        // Get check id from the end and remove it from todo
        id = _otherChecksTodo.back();
        _otherChecksTodo.pop_back();

        // Add the id to the list sent in this cycle
        _currentChecks.push_back(id);

        wd = sWardenCheckMgr->GetWardenDataById(id);

        switch (wd->Type)
        {
            case MPQ_CHECK:
            case LUA_STR_CHECK:
            case DRIVER_CHECK:
                buff << uint8(wd->Str.size());
                buff.append(wd->Str.c_str(), wd->Str.size());
                break;
            default:
                break;
        }
    }

    uint8 xorByte = _inputKey[0];

    // Add TIMING_CHECK
    buff << uint8(0x00);
    buff << uint8(TIMING_CHECK ^ xorByte);

    uint8 index = 1;

    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        wd = sWardenCheckMgr->GetWardenDataById(*itr);

        type = wd->Type;
        buff << uint8(type ^ xorByte);
        switch (type)
        {
            case MEM_CHECK:
            {
                buff << uint8(0x00);
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            {
                std::vector<uint8> data = wd->Data.ToByteVector(0, false);
                buff.append(data.data(), data.size());
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }
            case MPQ_CHECK:
            case LUA_STR_CHECK:
            {
                buff << uint8(index++);
                break;
            }
            case DRIVER_CHECK:
            {
                std::vector<uint8> data = wd->Data.ToByteVector(0, false);
                buff.append(data.data(), data.size());
                buff << uint8(index++);
                break;
            }
            case MODULE_CHECK:
            {
                std::array<uint8, 4> seed = SkyFire::Crypto::GetRandomBytes<4>();
                buff.append(seed);
                buff.append(SkyFire::Crypto::HMAC_SHA1::GetDigestOf(seed, wd->Str));
                break;
            }
            /*case PROC_CHECK:
            {
                buff.append(wd->i.AsByteArray(0, false).get(), wd->i.GetNumBytes());
                buff << uint8(index++);
                buff << uint8(index++);
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }*/
            default:
                break;                                      // Should never happen
        }
    }
    buff << uint8(xorByte);
    buff.hexlike();

    // Encrypt with warden RC4 key
    EncryptData(buff.contents(), buff.size());

    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size());
    pkt.append(buff);
    _session->SendPacket(&pkt);

    _dataSent = true;

    std::stringstream stream;
    stream << "Sent check id's: ";
    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
        stream << *itr << " ";

    SF_LOG_DEBUG("warden", "%s", stream.str().c_str());
}

void WardenWin::HandleData(ByteBuffer& buff)
{
    SF_LOG_DEBUG("warden", "Handle data");

    _dataSent = false;
    _clientResponseTimer = 0;

    uint16 Length;
    buff >> Length;
    uint32 Checksum;
    buff >> Checksum;

    if (!IsValidCheckSum(Checksum, buff.contents() + buff.rpos(), Length))
    {
        buff.rpos(buff.wpos());
        SF_LOG_WARN("warden", "%s failed checksum. Action: %s", _session->GetPlayerInfo().c_str(), Penalty().c_str());
        return;
    }

    // TIMING_CHECK
    {
        uint8 result;
        buff >> result;
        /// @todo test it.
        if (result == 0x00)
        {
            SF_LOG_WARN("warden", "%s failed timing check. Action: %s", _session->GetPlayerInfo().c_str(), Penalty().c_str());
            return;
        }

        uint32 newClientTicks;
        buff >> newClientTicks;

        uint32 ticksNow = getMSTime();
        uint32 ourTicks = newClientTicks + (ticksNow - _serverTicks);

        SF_LOG_DEBUG("warden", "ServerTicks %u", ticksNow);         // Now
        SF_LOG_DEBUG("warden", "RequestTicks %u", _serverTicks);    // At request
        SF_LOG_DEBUG("warden", "Ticks %u", newClientTicks);         // At response
        SF_LOG_DEBUG("warden", "Ticks diff %u", ourTicks - newClientTicks);
    }

    WardenCheckResult* rs;
    WardenCheck* rd;
    uint8 type;
    uint16 checkFailed = 0;

    ACE_READ_GUARD(ACE_RW_Mutex, g, sWardenCheckMgr->_checkStoreLock);

    for (std::list<uint16>::iterator itr = _currentChecks.begin(); itr != _currentChecks.end(); ++itr)
    {
        rd = sWardenCheckMgr->GetWardenDataById(*itr);
        rs = sWardenCheckMgr->GetWardenResultById(*itr);

        type = rd->Type;
        switch (type)
        {
            case MEM_CHECK:
            {
                uint8 Mem_Result;
                buff >> Mem_Result;

                if (Mem_Result != 0)
                {
                    SF_LOG_DEBUG("warden", "RESULT MEM_CHECK not 0x00, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    checkFailed = *itr;
                    continue;
                }

                std::vector<uint8> result = rs->Result.ToByteVector(0, false);
                if (memcmp(buff.contents() + buff.rpos(), result.data(), rd->Length) != 0)
                {
                    SF_LOG_DEBUG("warden", "RESULT MEM_CHECK fail CheckId %u account Id %u", *itr, _session->GetAccountId());
                    checkFailed = *itr;
                    buff.rpos(buff.rpos() + rd->Length);
                    continue;
                }

                buff.rpos(buff.rpos() + rd->Length);
                SF_LOG_DEBUG("warden", "RESULT MEM_CHECK passed CheckId %u account Id %u", *itr, _session->GetAccountId());
                break;
            }
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            case DRIVER_CHECK:
            case MODULE_CHECK:
            {
                const uint8 byte = 0xE9;
                if (memcmp(buff.contents() + buff.rpos(), &byte, sizeof(uint8)) != 0)
                {
                    if (type == PAGE_CHECK_A || type == PAGE_CHECK_B)
                        SF_LOG_DEBUG("warden", "RESULT PAGE_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    if (type == MODULE_CHECK)
                        SF_LOG_DEBUG("warden", "RESULT MODULE_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    if (type == DRIVER_CHECK)
                        SF_LOG_DEBUG("warden", "RESULT DRIVER_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    checkFailed = *itr;
                    buff.rpos(buff.rpos() + 1);
                    continue;
                }

                buff.rpos(buff.rpos() + 1);
                if (type == PAGE_CHECK_A || type == PAGE_CHECK_B)
                    SF_LOG_DEBUG("warden", "RESULT PAGE_CHECK passed CheckId %u account Id %u", *itr, _session->GetAccountId());
                else if (type == MODULE_CHECK)
                    SF_LOG_DEBUG("warden", "RESULT MODULE_CHECK passed CheckId %u account Id %u", *itr, _session->GetAccountId());
                else if (type == DRIVER_CHECK)
                    SF_LOG_DEBUG("warden", "RESULT DRIVER_CHECK passed CheckId %u account Id %u", *itr, _session->GetAccountId());
                break;
            }
            case LUA_STR_CHECK:
            {
                uint8 Lua_Result;
                buff >> Lua_Result;

                if (Lua_Result != 0)
                {
                    SF_LOG_DEBUG("warden", "RESULT LUA_STR_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    checkFailed = *itr;
                    continue;
                }

                uint8 luaStrLen;
                buff >> luaStrLen;

                if (luaStrLen != 0)
                {
                    char* str = new char[luaStrLen + 1];
                    memcpy(str, buff.contents() + buff.rpos(), luaStrLen);
                    str[luaStrLen] = '\0'; // null terminator
                    SF_LOG_DEBUG("warden", "Lua string: %s", str);
                    delete[] str;
                }
                buff.rpos(buff.rpos() + luaStrLen);         // Skip string
                SF_LOG_DEBUG("warden", "RESULT LUA_STR_CHECK passed, CheckId %u account Id %u", *itr, _session->GetAccountId());
                break;
            }
            case MPQ_CHECK:
            {
                uint8 Mpq_Result;
                buff >> Mpq_Result;

                if (Mpq_Result != 0)
                {
                    SF_LOG_DEBUG("warden", "RESULT MPQ_CHECK not 0x00 account id %u", _session->GetAccountId());
                    checkFailed = *itr;
                    continue;
                }

                if (memcmp(buff.contents() + buff.rpos(), rs->Result.ToByteArray<20>(false).data(), SkyFire::Crypto::Constants::SHA1_DIGEST_LENGTH_BYTES) != 0) // SHA1
                {
                    SF_LOG_DEBUG("warden", "RESULT MPQ_CHECK fail, CheckId %u account Id %u", *itr, _session->GetAccountId());
                    checkFailed = *itr;
                    buff.rpos(buff.rpos() + SkyFire::Crypto::Constants::SHA1_DIGEST_LENGTH_BYTES);            // 20 bytes SHA1
                    continue;
                }

                buff.rpos(buff.rpos() + SkyFire::Crypto::Constants::SHA1_DIGEST_LENGTH_BYTES);                // 20 bytes SHA1
                SF_LOG_DEBUG("warden", "RESULT MPQ_CHECK passed, CheckId %u account Id %u", *itr, _session->GetAccountId());
                break;
            }
            default:                                        // Should never happen
                break;
        }
    }

    if (checkFailed > 0)
    {
        WardenCheck* check = sWardenCheckMgr->GetWardenDataById(checkFailed);
        SF_LOG_WARN("warden", "%s failed Warden check %u. Action: %s", _session->GetPlayerInfo().c_str(), checkFailed, Penalty(check).c_str());
    }

    // Set hold off timer, minimum timer should at least be 1 second
    uint32 holdOff = sWorld->getIntConfig(WorldIntConfigs::CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF);
    _checkTimer = (holdOff < 1 ? 1 : holdOff) * IN_MILLISECONDS;
}
