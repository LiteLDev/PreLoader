#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct ap_hash {
    size_t operator()(const std::string& x) const {
        uint64_t rval = 0;
        for (size_t i = 0; x[i]; ++i) {
            if (i & 1) {
                rval ^= (~((rval << 11) ^ x[i] ^ (rval >> 5)));
            } else {
                rval ^= (~((rval << 7) ^ x[i] ^ (rval >> 3)));
            }
        }
        return rval;
    }
};
