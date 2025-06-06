/*
* This file is part of Project SkyFire https://www.projectskyfire.org.
* See LICENSE.md file for Copyright information
*/

#include <cstring>
#include "CryptoHash.h"

#ifndef _SESSIONKEYGENERATOR_HPP
#define _SESSIONKEYGENERATOR_HPP

template <typename Hash>
class SessionKeyGenerator
{
    public:
        template <typename C>
        SessionKeyGenerator(C const& buf) :
            o0it(o0.begin())
        {
            uint8 const* data = std::data(buf);
            size_t const len = std::size(buf);
            size_t const halflen = (len / 2);

            o1 = Hash::GetDigestOf(data, halflen);
            o2 = Hash::GetDigestOf(data + halflen, len - halflen);
            o0 = Hash::GetDigestOf(o1, o0, o2);
        }

        void Generate(uint8* buf, uint32 sz)
        {
            for (uint32 i = 0; i < sz; ++i)
            {
                if (o0it == o0.end())
                {
                    o0 = Hash::GetDigestOf(o1, o0, o2);
                    o0it = o0.begin();
                }

                buf[i] = *(o0it++);
            }
        }

    private:
        typename Hash::Digest o0 = { };
        typename Hash::Digest o1 = { };
        typename Hash::Digest o2 = { };
        typename Hash::Digest::const_iterator o0it;
    };

#endif
