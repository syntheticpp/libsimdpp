/*  libsimdpp
    Copyright (C) 2012  Povilas Kanapickas tir5c3@yahoo.co.uk
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#include "test_case.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <typeinfo>
#include <cstdlib>

TestCase::TestCase(const char* name, const char* file) :
    name_(name),
    file_(file),
    curr_precision_ulp_(0)
{
    reset_seq();
}

void TestCase::push(Type type, void* data, unsigned line)
{
    results_.push_back(Result(type, data, size_for_type(type), line, seq_++,
                              curr_precision_ulp_));
}

std::size_t TestCase::size_for_type(Type t)
{
    switch (t) {
    case TYPE_UINT16:   return 2;
    case TYPE_UINT8x16:
    case TYPE_INT8x16:
    case TYPE_UINT16x8:
    case TYPE_INT16x8:
    case TYPE_UINT32x4:
    case TYPE_INT32x4:
    case TYPE_UINT64x2:
    case TYPE_INT64x2:
    case TYPE_FLOAT32x4:
    case TYPE_FLOAT64x2: return 16;
    case TYPE_UINT8x32:
    case TYPE_INT8x32:
    case TYPE_UINT16x16:
    case TYPE_INT16x16:
    case TYPE_UINT32x8:
    case TYPE_INT32x8:
    case TYPE_UINT64x4:
    case TYPE_INT64x4:
    case TYPE_FLOAT32x8:
    case TYPE_FLOAT64x4: return 32;
    default: std::abort();
    }
}

unsigned TestCase::precision_for_result(const Result& res)
{
    switch (res.type) {
    case TestCase::TYPE_FLOAT32x4:
    case TestCase::TYPE_FLOAT64x2:
    case TestCase::TYPE_FLOAT32x8:
    case TestCase::TYPE_FLOAT64x4:
        return res.prec_ulp;
    default:
        return 0;
    }
}

template<class T> struct fix_char_type { typedef T type; };
template<> struct fix_char_type<uint8_t> { typedef int type; };
template<> struct fix_char_type<int8_t> { typedef int type; };

template<unsigned width, unsigned num_elems, class T>
void fmt_vec_hex(std::ostream& err, const char* prefix, const T* p)
{
    err << prefix << "[ " << std::hex << std::setfill('0');
    err.precision(width);
    for (unsigned i = 0; i < num_elems; i++, p++) {
        // chars need to be converted to some other type
        err << std::setw(width*2) << uint64_t(*p);
        if (i != num_elems - 1) {
            err << " ; ";
        }
    }
    err << " ]\n";
    err << std::dec << std::setfill(' ');
}

template<unsigned precision, unsigned num_elems, class T>
void fmt_vec_num(std::ostream& err, const char* prefix, const T* p)
{
    err << prefix << "[ ";
    err.precision(precision);
    for (unsigned i = 0; i < num_elems; i++, p++) {
        err << typename fix_char_type<T>::type(*p);
        if (i != num_elems - 1) {
            err << " ; ";
        }
    }
    err << " ]\n";
    err << std::dec;
}

template<class T>
void fmt_bin(std::ostream& err, const char* prefix, const T& n)
{
    err << prefix << "[ ";
    unsigned bits = sizeof(T)*8;
    for (unsigned i = 0; i < bits; i++) {
        err << ((n & (1 << i)) ? '1' : '0');
    }
    err << " ]\n";
    err << std::dec;
}

template<unsigned num_elems, class T>
bool cmpeq_arrays(const T* a, const T* b, unsigned prec)
{
    for (unsigned i = 0; i < num_elems; i++) {
        // we need to be extra-precise here. nextafter works won't introduce
        // any rounding errors
        T ia = *a++;
        T ib = *b++;
        for (unsigned i = 0; i < prec; i++) {
            ia = nextafter(ia, ib);
        }
        if (ia != ib) {
            return false;
        }
    }
    return true;
}

bool test_equal(const TestCase& a, const char* a_arch,
                const TestCase& b, const char* b_arch,
                std::ostream& err)

{
    struct TestEqualImpl {

    const TestCase& a;
    const char* a_arch;
    const TestCase& b;
    const char* b_arch;
    std::ostream& err;

    TestEqualImpl(const TestCase& d_a, const char* d_a_arch,
                  const TestCase& d_b, const char* d_b_arch,
                  std::ostream& d_err) :
        a(d_a), a_arch(d_a_arch), b(d_b), b_arch(d_b_arch), err(d_err)
    {
    }

    void fmt_separator()
    {
        err << "--------------------------------------------------------------\n";
    }
    void fmt_arch()
    {
        err << "  For architectures: " << a_arch << " and " << b_arch << " :\n";
    }
    void fmt_file()
    {
        fmt_arch();
        err << "  In file \"" << a.file_ << "\" :\n";
    }
    void fmt_file_line(unsigned line)
    {
        fmt_arch();
        err << "  In file \"" << a.file_ << "\" at line " << line << " : \n";
    }
    void fmt_test_case()
    {
        err << "  In test case \"" << a.name_ << "\" :\n";
    }
    void fmt_seq(unsigned num)
    {
        err << "  Sequence number: " << num << "\n"; // start from one
    }
    void fmt_prec(unsigned prec)
    {
        if (prec > 0) {
            err << "  Precision: " << prec << "ULP\n";
        }
    }

    const char* type_str(unsigned type)
    {
        switch (type) {
        case TestCase::TYPE_UINT16: return "uint16";
        case TestCase::TYPE_UINT8x16: return "uint8x16";
        case TestCase::TYPE_INT8x16: return "int8x16";
        case TestCase::TYPE_UINT16x8: return "uint16x8";
        case TestCase::TYPE_INT16x8: return "int16x8";
        case TestCase::TYPE_UINT32x4: return "uint32x4";
        case TestCase::TYPE_INT32x4: return "int32x4";
        case TestCase::TYPE_UINT64x2: return "uint64x2";
        case TestCase::TYPE_INT64x2: return "int64x2";
        case TestCase::TYPE_FLOAT32x4: return "float32x4";
        case TestCase::TYPE_FLOAT64x2: return "float64x2";
        case TestCase::TYPE_UINT8x32: return "uint8x32";
        case TestCase::TYPE_INT8x32: return "int8x32";
        case TestCase::TYPE_UINT16x16: return "uint16x16";
        case TestCase::TYPE_INT16x16: return "int16x16";
        case TestCase::TYPE_UINT32x8: return "uint32x8";
        case TestCase::TYPE_INT32x8: return "int32x8";
        case TestCase::TYPE_UINT64x4: return "uint64x4";
        case TestCase::TYPE_INT64x4: return "int64x4";
        case TestCase::TYPE_FLOAT32x8: return "float32x8";
        case TestCase::TYPE_FLOAT64x4: return "float64x4";
        default: return "UNDEFINED";
        }
    }
    void fmt_vector(const TestCase::Result& r, const char* prefix)
    {
        switch (r.type) {
        case TestCase::TYPE_UINT16:
            fmt_bin(err, prefix, r.u16);
            break;
        case TestCase::TYPE_UINT8x16:
            fmt_vec_hex<1,16>(err, prefix, r.v_u8);
            fmt_vec_num<4,16>(err, prefix, r.v_u8);
            break;
        case TestCase::TYPE_INT8x16:
            fmt_vec_hex<1,16>(err, prefix, r.v_s8);
            fmt_vec_num<4,16>(err, prefix, r.v_s8);
            break;
        case TestCase::TYPE_UINT16x8:
            fmt_vec_hex<2,8>(err, prefix, r.v_u16);
            fmt_vec_num<6,8>(err, prefix, r.v_u16);
            break;
        case TestCase::TYPE_INT16x8:
            fmt_vec_hex<2,8>(err, prefix, r.v_s16);
            fmt_vec_num<6,8>(err, prefix, r.v_s16);
            break;
        case TestCase::TYPE_UINT32x4:
            fmt_vec_hex<4,4>(err, prefix, r.v_u32);
            fmt_vec_num<11,4>(err, prefix, r.v_u32);
            break;
        case TestCase::TYPE_INT32x4:
            fmt_vec_hex<4,4>(err, prefix, r.v_s32);
            fmt_vec_num<11,4>(err, prefix, r.v_s32);
            break;
        case TestCase::TYPE_UINT64x2:
            fmt_vec_hex<8,2>(err, prefix, r.v_u64);
            fmt_vec_num<20,2>(err, prefix, r.v_u64);
            break;
        case TestCase::TYPE_INT64x2:
            fmt_vec_hex<8,2>(err, prefix, r.v_s64);
            fmt_vec_num<20,2>(err, prefix, r.v_s64);
            break;
        case TestCase::TYPE_FLOAT32x4:
            fmt_vec_hex<4,4>(err, prefix, r.v_u32);
            fmt_vec_num<7,4>(err, prefix, r.v_f32);
            break;
        case TestCase::TYPE_FLOAT64x2:
            fmt_vec_hex<8,2>(err, prefix, r.v_u64);
            fmt_vec_num<17,2>(err, prefix, r.v_f64);
            break;
        // 32-byte vectors
        case TestCase::TYPE_UINT8x32:
            fmt_vec_hex<1,32>(err, prefix, r.v_u8);
            fmt_vec_num<4,32>(err, prefix, r.v_u8);
            break;
        case TestCase::TYPE_INT8x32:
            fmt_vec_hex<1,32>(err, prefix, r.v_s8);
            fmt_vec_num<4,32>(err, prefix, r.v_s8);
            break;
        case TestCase::TYPE_UINT16x16:
            fmt_vec_hex<2,16>(err, prefix, r.v_u16);
            fmt_vec_num<6,16>(err, prefix, r.v_u16);
            break;
        case TestCase::TYPE_INT16x16:
            fmt_vec_hex<2,16>(err, prefix, r.v_s16);
            fmt_vec_num<6,16>(err, prefix, r.v_s16);
            break;
        case TestCase::TYPE_UINT32x8:
            fmt_vec_hex<4,8>(err, prefix, r.v_u32);
            fmt_vec_num<11,8>(err, prefix, r.v_u32);
            break;
        case TestCase::TYPE_INT32x8:
            fmt_vec_hex<4,8>(err, prefix, r.v_s32);
            fmt_vec_num<11,8>(err, prefix, r.v_s32);
            break;
        case TestCase::TYPE_UINT64x4:
            fmt_vec_hex<8,4>(err, prefix, r.v_u64);
            fmt_vec_num<20,4>(err, prefix, r.v_u64);
            break;
        case TestCase::TYPE_INT64x4:
            fmt_vec_hex<8,4>(err, prefix, r.v_s64);
            fmt_vec_num<20,4>(err, prefix, r.v_s64);
            break;
        case TestCase::TYPE_FLOAT32x8:
            fmt_vec_hex<4,8>(err, prefix, r.v_u32);
            fmt_vec_num<7,8>(err, prefix, r.v_f32);
            break;
        case TestCase::TYPE_FLOAT64x4:
            fmt_vec_hex<8,4>(err, prefix, r.v_u64);
            fmt_vec_num<17,4>(err, prefix, r.v_f64);
            break;
        }
    }

    bool cmpeq_result(const TestCase::Result& ia, const TestCase::Result& ib,
                      unsigned prec)
    {
        if (std::memcmp(ia.v_u8, ib.v_u8, TestCase::size_for_type(ia.type)) == 0) {
            return true;
        }

        if (prec == 0) {
            return false;
        }

        switch (ia.type) {
        case TestCase::TYPE_FLOAT32x4:
            return cmpeq_arrays<4>(ia.v_f32, ib.v_f32, prec);
        case TestCase::TYPE_FLOAT64x2:
            return cmpeq_arrays<2>(ia.v_f64, ib.v_f64, prec);
        case TestCase::TYPE_FLOAT32x8:
            return cmpeq_arrays<8>(ia.v_f32, ib.v_f32, prec);
        case TestCase::TYPE_FLOAT64x4:
            return cmpeq_arrays<4>(ia.v_f64, ib.v_f64, prec);
        default:
            return false;
        }
    }

    bool run()
    {
        // Handle fatal errors first
        if (std::strcmp(a.name_, b.name_) != 0) {
            fmt_separator();
            fmt_file();
            err << "FATAL: Test case names do not match: \""
                << a.name_ << "\" and \""  << b.name_ << "\"\n";
            fmt_separator();
            return false;
        }

        if (a.results_.size() != b.results_.size()) {
            fmt_separator();
            fmt_file();
            fmt_test_case();
            err << "FATAL: The lengths of the result vectors does not match: "
                << a.results_.size() << "/" << b.results_.size() << "\n";
            fmt_separator();
            return false;
        }

        bool ok = true;
        // Compare results
        for (unsigned i = 0; i < a.results_.size(); i++) {
            const TestCase::Result& ia = a.results_[i];
            const TestCase::Result& ib = b.results_[i];

            if (ia.line != ib.line) {
                fmt_separator();
                fmt_file();
                fmt_test_case();
                err << "FATAL: Line numbers do not match for items with the same "
                    << "sequence number: id: " << i
                    << " line_A: " << ia.line << " line_B: " << ib.line << "\n";
                fmt_separator();
                return false;
            }

            if (ia.type != ib.type) {
                fmt_separator();
                fmt_file_line(ia.line);
                fmt_test_case();
                err << "FATAL: Types do not match for items with the same "
                    << "sequence number: id: " << i
                    << " type_A: " << type_str(ia.type)
                    << " line_B: " << type_str(ib.type) << "\n";
                fmt_separator();
                return false;
            }

            unsigned prec = std::max(TestCase::precision_for_result(ia),
                                     TestCase::precision_for_result(ib));

            if (!cmpeq_result(ia, ib, prec)) {
                fmt_separator();
                fmt_file_line(ia.line);
                fmt_test_case();
                fmt_seq(ia.seq);
                err << "ERROR: Vectors not equal: \n";
                fmt_vector(ia, "A : ");
                fmt_vector(ib, "B : ");
                fmt_prec(prec);
                fmt_separator();
                ok = false;
            }
        }
        return ok;
    }
    }; // end TestEqualImpl
    TestEqualImpl tst(a, a_arch, b, b_arch, err);
    return tst.run();
}
