// Copyright (c) 2017, SUMOKOIN
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#include <malloc.h>
#include <intrin.h>
#define HAS_WIN_INTRIN_API
#endif

// Note HAS_INTEL_HW and future HAS_ARM_HW only mean we can emit the AES instructions
// check CPU support for the hardware AES encryption has to be done at runtime
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X86) || defined(_M_X64)
#ifdef __GNUC__
#include <x86intrin.h>
#pragma GCC target ("aes")
#if !defined(HAS_WIN_INTRIN_API)
#include <cpuid.h>
#endif // !defined(HAS_WIN_INTRIN_API)
#endif // __GNUC__
#define HAS_INTEL_HW
#endif

#import "mm_malloc.h"

namespace cn_heavy {

    inline bool hw_check_aes()
    {
        return false;
    }


// This cruft avoids casting-galore and allows us not to worry about sizeof(void*)
class cn_sptr
{
public:
	cn_sptr() : base_ptr(nullptr) {}
	cn_sptr(uint64_t* ptr) { base_ptr = ptr; }
	cn_sptr(uint32_t* ptr) { base_ptr = ptr; }
	cn_sptr(uint8_t* ptr) { base_ptr = ptr; }

	inline void set(void* ptr) { base_ptr = ptr; }
	inline cn_sptr offset(size_t i) { return reinterpret_cast<uint8_t*>(base_ptr)+i; }
	inline const cn_sptr offset(size_t i) const { return reinterpret_cast<uint8_t*>(base_ptr)+i; }

	inline void* as_void() { return base_ptr; }
	inline uint8_t& as_byte(size_t i) { return *(reinterpret_cast<uint8_t*>(base_ptr)+i); }
	inline uint8_t* as_byte() { return reinterpret_cast<uint8_t*>(base_ptr); }
	inline uint64_t& as_uqword(size_t i) { return *(reinterpret_cast<uint64_t*>(base_ptr)+i); }
	inline const uint64_t& as_uqword(size_t i) const { return *(reinterpret_cast<uint64_t*>(base_ptr)+i); } 
	inline uint64_t* as_uqword() { return reinterpret_cast<uint64_t*>(base_ptr); }
	inline const uint64_t* as_uqword() const { return reinterpret_cast<uint64_t*>(base_ptr); }
	inline int64_t& as_qword(size_t i) { return *(reinterpret_cast<int64_t*>(base_ptr)+i); }
	inline int32_t& as_dword(size_t i) { return *(reinterpret_cast<int32_t*>(base_ptr)+i); }
	inline uint32_t& as_udword(size_t i) { return *(reinterpret_cast<uint32_t*>(base_ptr)+i); }
	inline const uint32_t& as_udword(size_t i) const { return *(reinterpret_cast<uint32_t*>(base_ptr)+i); }
private:
	void* base_ptr;
};
    
template<size_t MEMORY, size_t ITER, size_t VERSION>
class cn_slow_hash
{
public:
	cn_slow_hash()
	{
        lpad.set(_mm_malloc(MEMORY, 4096));
        spad.set(_mm_malloc(4096, 4096));
	}

	cn_slow_hash (cn_slow_hash&& other) noexcept : lpad(other.lpad.as_byte()), spad(other.spad.as_byte()) 
	{
		other.lpad.set(nullptr);
		other.spad.set(nullptr);
	}
	
	cn_slow_hash& operator= (cn_slow_hash&& other) noexcept
    {
		if(this == &other)
			return *this;

		free_mem();
		lpad.set(other.lpad.as_void());
		spad.set(other.spad.as_void());
		return *this;
	}

	// Copying is going to be really inefficient
	cn_slow_hash(const cn_slow_hash& other) = delete;
	cn_slow_hash& operator= (const cn_slow_hash& other) = delete;

	~cn_slow_hash()
	{
		free_mem();
	}

	void hash(const void* in, size_t len, void* out)
	{
        software_hash(in, len, out);
	}

	void software_hash(const void* in, size_t len, void* out);
	
private:
	static constexpr size_t MASK = ((MEMORY-1) >> 4) << 4;

	inline bool check_override()
	{
		const char *env = getenv("SUMO_USE_SOFTWARE_AES");
		if (!env) {
			return false;
		}
		else if (!strcmp(env, "0") || !strcmp(env, "no")) {
			return false;
		}
		else {
			return true;
		}
	}
	
	inline void free_mem()
	{
        if(lpad.as_void() != nullptr)
            free(lpad.as_void());
        if(lpad.as_void() != nullptr)
            free(spad.as_void());
        
		lpad.set(nullptr);
		spad.set(nullptr);
	}

	inline cn_sptr scratchpad_ptr(uint32_t idx) { return lpad.as_byte() + (idx & MASK); }

    inline void explode_scratchpad_hard() { assert(false); }
    inline void implode_scratchpad_hard() { assert(false); }

	void explode_scratchpad_soft();
	void implode_scratchpad_soft();

	cn_sptr lpad;
	cn_sptr spad;
};

using cn_pow_hash_v1 = cn_slow_hash<2*1024*1024, 0x80000, 0>;
using cn_pow_hash_v2 = cn_slow_hash<4*1024*1024, 0x40000, 1>;

extern template class cn_slow_hash<2*1024*1024, 0x80000, 0>;
extern template class cn_slow_hash<4*1024*1024, 0x40000, 1>;

} //cn_heavy namespace

