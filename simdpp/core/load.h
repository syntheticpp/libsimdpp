/*  Copyright (C) 2013-2014  Povilas Kanapickas <povilas@radix.lt>

    Distributed under the Boost Software License, Version 1.0.
        (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef LIBSIMDPP_SIMDPP_CORE_LOAD_H
#define LIBSIMDPP_SIMDPP_CORE_LOAD_H

#ifndef LIBSIMDPP_SIMD_H
    #error "This file must be included through simd.h"
#endif

#include <simdpp/types.h>
#include <simdpp/detail/insn/load.h>

namespace simdpp {
#ifndef SIMDPP_DOXYGEN
namespace SIMDPP_ARCH_NAMESPACE {
#endif

/** Loads a 128-bit or 256-bit integer, 32-bit or 64-bit float vector
    from an aligned memory location.

    @par 128-bit version:

    @code
    a[0..127] = *(p)
    @endcode
    @a p must be aligned to 16 bytes.

    @par 256-bit version:

    @code
    a[0..255] = *(p)
    @endcode
    @a p must be aligned to 32 bytes.

    @icost{SSE2-SSE4.1, NEON, ALTIVEC, 2}
    @icost{AVX (integer vectors), 2}
*/
// Fixme return empty expression
template<class V = expr_vec_load> SIMDPP_INL
V load(const void* p)
{
    static_assert((is_vector<V>::value && !is_mask<V>::value) ||
                      detail::is_expr_vec_load<V>::value,
                  "V must be a non-mask vector");
    return detail::insn::i_load_dispatch<V>::run(reinterpret_cast<const char*>(p));
}

#ifndef SIMDPP_DOXYGEN
} // namespace SIMDPP_ARCH_NAMESPACE
#endif
} // namespace simdpp

#endif

