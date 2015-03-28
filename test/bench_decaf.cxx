/**
 * @file test_decaf.cxx
 * @author Mike Hamburg
 *
 * @copyright
 *   Copyright (c) 2015 Cryptography Research, Inc.  \n
 *   Released under the MIT License.  See LICENSE.txt for license information.
 *
 * @brief C++ benchmarks, because that's easier.
 */

#include "decaf.hxx"
#include "shake.h"
#include "decaf_crypto.h"
#include <stdio.h>
#include <sys/time.h>
#include <assert.h>
#include <stdint.h>

typedef decaf::decaf<448>::Scalar Scalar;
typedef decaf::decaf<448>::Point Point;
typedef decaf::decaf<448>::Precomputed Precomputed;

static __inline__ void __attribute__((unused)) ignore_result ( int result ) { (void)result; }
static double now(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + tv.tv_usec/1000000.0;
}

// RDTSC from the chacha code
#ifndef __has_builtin
#define __has_builtin(X) 0
#endif
#if defined(__clang__) && __has_builtin(__builtin_readcyclecounter)
#define rdtsc __builtin_readcyclecounter
#else
static inline uint64_t rdtsc(void) {
  u_int64_t out = 0;
# if (defined(__i386__) || defined(__x86_64__))
    __asm__ __volatile__ ("rdtsc" : "=A"(out));
# endif
  return out;
}
#endif

class Benchmark {
    static const int NTESTS = 1000;
public:
    int i, ntests;
    double begin;
    uint64_t tsc_begin;
    Benchmark(const char *s, double factor = 1) {
        printf("%s:", s);
        if (strlen(s) < 25) printf("%*s",int(25-strlen(s)),"");
        fflush(stdout);
        i = 0;
        ntests = NTESTS * factor;
        begin = now();
        tsc_begin = rdtsc();
    }
    ~Benchmark() {
        double tsc = (rdtsc() - tsc_begin) * 1.0 / ntests;
        double t = (now() - begin)/ntests;
        const char *small[] = {" ","m","µ","n","p"};
        const char *big[] = {" ","k","M","G","T"};
        unsigned di=0;
        for (di=0; di<sizeof(small)/sizeof(*small)-1 && t && t < 1; di++) {
            t *= 1000.0;
        }
        unsigned bi=0;
        for (bi=0; bi<sizeof(big)/sizeof(*big)-1 && tsc && tsc >= 1000; bi++) {
            tsc /= 1000.0;
        }
        printf("%7.2f %ss", t, small[di]);
        if (tsc) printf("   %7.2f %scy", tsc, big[bi]);
        printf("\n");
    }
    inline bool iter() { return i++ < ntests; }
};

int main(int argc, char **argv) {
    bool micro = false;
    if (argc >= 2 && !strcmp(argv[1], "--micro"))
        micro = true;
    
    decaf_448_public_key_t p1,p2;
    decaf_448_private_key_t s1,s2;
    decaf_448_symmetric_key_t r1,r2;
    decaf_448_signature_t sig1;
    unsigned char ss[32];
    
    memset(r1,1,sizeof(r1));
    memset(r2,2,sizeof(r2)); 
    
    unsigned char umessage[] = {1,2,3,4,5};
    size_t lmessage = sizeof(umessage);
    

    if (micro) {
        Precomputed pBase;
        Point p,q;
        Scalar s,t;
        
        printf("Micro:\n");
        for (Benchmark b("Scalar add", 1000); b.iter(); ) { s+t; }
        for (Benchmark b("Scalar times", 100); b.iter(); ) { s*t; }
        for (Benchmark b("Scalar inv", 10); b.iter(); ) { s.inverse(); }
        for (Benchmark b("Point add", 100); b.iter(); ) { p + q; }
        for (Benchmark b("Point double", 100); b.iter(); ) { p.double_in_place(); }
        for (Benchmark b("Point scalarmul"); b.iter(); ) { p * s; }
        for (Benchmark b("Point double scalarmul"); b.iter(); ) { Point::double_scalarmul(p,s,q,t); }
        for (Benchmark b("Point precmp scalarmul"); b.iter(); ) { pBase * s; }
        /* TODO: scalarmul for verif */
        printf("\nMacro:\n");
    }
    
    for (Benchmark b("Keygen"); b.iter(); ) {
        decaf_448_derive_private_key(s1,r1);
    }
    
    decaf_448_private_to_public(p1,s1);
    decaf_448_derive_private_key(s2,r2);
    decaf_448_private_to_public(p2,s2);
    
    for (Benchmark b("Shared secret"); b.iter(); ) {
        decaf_bool_t ret = decaf_448_shared_secret(ss,sizeof(ss),s1,p2);
        ignore_result(ret);
        assert(ret);
    }
    
    for (Benchmark b("Sign"); b.iter(); ) {
        decaf_448_sign(sig1,s1,umessage,lmessage);
    }
    
    for (Benchmark b("Verify"); b.iter(); ) {
        decaf_bool_t ret = decaf_448_verify(sig1,p1,umessage,lmessage);
        umessage[0]++;
        ignore_result(ret);
    }
    
    printf("\n");
    
    return 0;
}