/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created: 5/30/2017 lixun910@gmail.com
 *
 * title = "xoroshiro+ / xorshift* / xorshift+ generators and the PRNG shootout", //
 * booktitle = "Online", //
 * url = "http://xoroshiro.di.unimi.it/", //
 */
#ifndef __GEODA_CENTER_RNG_H__
#define __GEODA_CENTER_RNG_H__

#include <boost/unordered_map.hpp>

class Xoroshiro128Random
{
    long long s0;
    long long s1;
public:
    Xoroshiro128Random(long long xor64 = 123456789) {
        SetSeed(xor64);
    }

    virtual ~Xoroshiro128Random() {}

    void SetSeed(long long xor64 = 123456789) {
        // set seed
        // XorShift64* generator to seed:
        if (xor64 == 0)
            xor64 = 4101842887655102017L;
        xor64 ^= (unsigned long long)xor64 >> 12; // a
        xor64 ^= xor64 << 25; // b
        xor64 ^= (unsigned long long)xor64 >> 27; // c
        s0 = xor64 * 2685821657736338717L;
        xor64 ^= (unsigned long long)xor64 >> 12; // a
        xor64 ^= xor64 << 25; // b
        xor64 ^= (unsigned long long)xor64 >> 27; // c
        s1 = xor64 * 2685821657736338717L;
    }
    int nextInt(int n) {
        if (n <=0) return 0;
        int r =  (int)((n & -n) == n ? nextLong() & n - 1 // power of two
                       : (unsigned long long)(((unsigned long long)nextLong() >> 32) * n) >> 32);
        return r;
    }

    long long nextLong() {
        long long t0 = s0, t1 = s1;
        long long result = t0 + t1;
        t1 ^= t0;
        // left rotate: (n << d)|(n >> (INT_BITS - d));
        s0 = (t0 << 55) | ((unsigned long long)t0 >> (64 - 55));
        s0 = s0 ^ t1 ^ (t1 << 14); // a, b
        s1 = (t1 << 36) | ((unsigned long long)t1 >> (64 -36));
        return result;
    }
    // return a uniform distributed double value between [0, 1]
    double nextDouble() {
#ifdef __WXOSX__
        return ((unsigned long long)nextLong() >> 11) * 0x1.0p-53;
#else
        char tempStr[] = "0x1.0p-53";
        double nd = std::strtod(tempStr, NULL);
        return ((unsigned long long)nextLong() >> 11) * nd;
#endif
    }
    
    std::vector<int> randomSample(int samplesize, int n)
    {
        std::vector<int> samples(samplesize);
        int i=0;
        boost::unordered_map<int, bool> sample_dict;
        boost::unordered_map<int, bool>::iterator it;
        while (sample_dict.size() < samplesize) {
            int rnd = nextInt(n);
            if (sample_dict.find(rnd) == sample_dict.end()) {
                samples[i++] = rnd;
            }
            sample_dict[rnd] = true;
        }
        return samples;
    }
};

#endif
