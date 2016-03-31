// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of // TODO: needs to adjusted for checkpoint checks, also see main.cpp
        (         0, hashGenesisBlock)
        (         10000, uint256("0x2c9d6d46eae696903ecb483c3844d0f2da1d620f11814f7daeacf2c540f83290"))
        (         50000, uint256("0x947907a479759b476383d72ea56ee7556db1503fa6701abc9cb2b6bb6abe3f34"))
        (         100000, uint256("0xa9fb76f092d0e8b20f702010c1d9795f86a295e110ea8ad128143c4d402b0698"))
        (         200000, uint256("0x5023af87e4217a7f4749d4b9c5a20667718771996c98b27d1553c6e86c141e44"))
        (         300000, uint256("0x62b9979d7bb64d0bb8d3be6e03216c7343c19505f82ba4686b98caf789113829"))
        (         500000, uint256("0x72d62854ca3f3f14c057794d4c7cb371685631451744441018e7b62a34b7216e"))
        (         700000, uint256("0xde23d3472dfc9dab03a125265f2b8b034b74721ef6c878134a6f158634fe4d26"))
        (         900000, uint256("0x419d9f142e11b8cd8a117edff3c3531cc37d5cadf1cdd92523ea792e9da36d33"))
        (         1100000, uint256("0xe6cc1e31ca3f03c5f590c7b3591ca803c6383eeefb895301200a400b66f53d66"))
        (         1400000, uint256("0x0f4a5e63899d9436b0325d249ccc481be39cd10b0271cd1bf6b25d927256202b"))
        (         1800000, uint256("0xa968a168bdad10f833425f6e069712b0484d36a9a1d2f498303bb7ed6ccf8c9c"))
        (         2050000, uint256("0x4e32534fd14c2d04ff2795b1d76856b2dd0b44b0ca9ee33e9fbfb933caec7209"))
        (         2054436, uint256("0xa13bf7ffb832c1af0f538ab79c1aa909307580082270c2fa89dcdd119a204b56"))
        ;

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints

        MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
        if (i == mapCheckpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0;
        return mapCheckpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
