#ifndef CLZ_FALLBACK_H
#define CLZ_FALLBACK_H

#ifdef __TINYC__
    // 1. Provide the custom function implementation for TCC
    static inline int my_clz_fallback(unsigned int x) {
        if (x == 0) return 32; // Standard __builtin_clz(0) is undefined, but this is safe
        int count = 0;
        for (int i = 31; i >= 0; i--) {
            if ((x >> i) & 1) break;
            count++;
        }
        return count;
    }

    // 2. Map the GCC builtin to your fallback function
    #define __builtin_clz(x) my_clz_fallback(x)

#endif // __TINYC__

#endif // CLZ_FALLBACK_H
