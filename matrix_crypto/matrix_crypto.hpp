// matrix_crypto.hpp — математическое ядро матричной криптосистемы (порт Python-версии v2).
//
// Система работает в циклической подгруппе <T> ПРОСТОГО порядка q группы SL_3(F_p).
// Параметры (p, q, T) берутся из сгенерированного offline matrix_params.hpp
// (их создаёт gen_cpp_params.py из matrix_params.py, поэтому Python и C++ используют
// ОДНИ И ТЕ ЖЕ параметры => версии совместимы на уровне протокола).
//
//   p — 1023-битное простое (модуль поля)            -> тип U1024
//   q — 256-битное простое (порядок <T>)             -> тип U256
//   T — генератор подгруппы, order(T) = q            -> Mat3 над F_p
//
// Все экспоненты (секреты, нонсы, хэш-скаляры) принадлежат [0, q-1].
//
// Форматы (полностью совместимы с Python matrix_crypto_core.py):
//   * элемент поля — ровно ELEMENT_BYTES = ceil(bits(p)/8) = 128 байт, big-endian;
//   * матрица       — 9 элементов подряд (1152 байта);
//   * hash_to_scalar — SHA-512 от конкатенации кусков, приведённый по модулю q.
#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "u128.hpp"
#include "ubig.hpp"

#include "matrix_params.hpp"
#include "sha2.hpp"

namespace mc {

// ---------------------------------------------------------------------------
// Типы целых из библиотеки bignum (иерархия удваивает разрядность)
// ---------------------------------------------------------------------------
using bignum::UBig;
using bignum::u128::U128;

using U256 = UBig<U128>;
using U512 = UBig<U256>;
using U1024 = UBig<U512>;
using U2048 = UBig<U1024>;
using U4096 = UBig<U2048>;
using U8192 = UBig<U4096>;
using U16384 = UBig<U8192>;

// Широкий тип для кодирования сообщения (в base-p "цифрах" на всё сообщение).
// U16384 хватает примерно на 1800 байт текста (см. MAX_MESSAGE_BYTES).
using Wide = U16384;
using WideHalf = U8192;

inline constexpr size_t MAX_MESSAGE_BYTES = 1800;

// ---------------------------------------------------------------------------
// Матрица 3x3 над F_p
// ---------------------------------------------------------------------------
struct Mat3
{
    U1024 v[9]; // строка за строкой: v[3*i + j]

    U1024 &at(int i, int j) { return v[3 * i + j]; }
    const U1024 &at(int i, int j) const { return v[3 * i + j]; }

    bool operator==(const Mat3 &o) const
    {
        for (int i = 0; i < 9; ++i)
            if (!(v[i] == o.v[i])) return false;
        return true;
    }
};

// ---------------------------------------------------------------------------
// Параметры системы (ленивая инициализация из строк matrix_params.hpp)
// ---------------------------------------------------------------------------
inline const U1024 &P()
{
    static const U1024 p{std::string_view(params::P_DEC)};
    return p;
}

inline const U256 &Q()
{
    static const U256 q{std::string_view(params::Q_DEC)};
    return q;
}

inline Mat3 make_T()
{
    Mat3 t;
    for (int i = 0; i < 9; ++i)
        t.v[i] = U1024{std::string_view(params::T_DEC[i])};
    return t;
}

inline const Mat3 &T()
{
    static const Mat3 t = make_T();
    return t;
}

inline const Mat3 &IDENTITY()
{
    static const Mat3 I = []()
    {
        Mat3 m;
        for (int i = 0; i < 9; ++i) m.v[i] = U1024{0};
        m.v[0] = m.v[4] = m.v[8] = U1024{1};
        return m;
    }();
    return I;
}

inline size_t element_bytes()
{
    static const size_t eb = (P().bit_width() + 7) / 8;
    return eb;
}

// ---------------------------------------------------------------------------
// Приведение типов по иерархии UBig
// ---------------------------------------------------------------------------

// Сужение до целевого типа T (берём младшие разряды по цепочке .low()).
// Корректно, когда значение заведомо помещается в T.
template <typename T, typename V>
inline T narrow_to(const V &v)
{
    if constexpr (std::is_same_v<T, V>)
        return v;
    else
        return narrow_to<T>(v.low());
}

// Расширение значения типа V до целевого типа T по цепочке конструкторов.
template <typename T, typename V>
inline T widen_to(const V &v)
{
    if constexpr (std::is_same_v<T, V>)
        return v;
    else if constexpr (std::is_same_v<typename T::half_type, V>)
        return T(v);
    else
        return T(widen_to<typename T::half_type, V>(v));
}

// ---------------------------------------------------------------------------
// Арифметика по модулю p
// ---------------------------------------------------------------------------
inline U1024 mod_mul(const U1024 &a, const U1024 &b)
{
    U2048 prod = U2048::mult_ext(a, b); // полное 2048-битное произведение
    return (prod / P()).second;          // быстрый путь: деление на "половинку"
}

inline U1024 mod_add(const U1024 &a, const U1024 &b)
{
    U1024 s = a + b; // a,b < p < 2^1023 => сумма < 2^1024, переполнения нет
    return (s >= P()) ? (s - P()) : s;
}

inline U1024 mod_sub(const U1024 &a, const U1024 &b)
{
    return (a >= b) ? (a - b) : (P() - (b - a));
}

// ---------------------------------------------------------------------------
// Матричная арифметика над F_p
// ---------------------------------------------------------------------------
inline Mat3 mat_mul(const Mat3 &A, const Mat3 &B)
{
    Mat3 C;
    for (int i = 0; i < 9; ++i) C.v[i] = U1024{0};
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k)
        {
            const U1024 &a = A.at(i, k);
            if (!static_cast<bool>(a)) continue; // a == 0
            for (int j = 0; j < 3; ++j)
            {
                U1024 &c = C.at(i, j);
                c = mod_add(c, mod_mul(a, B.at(k, j)));
            }
        }
    return C;
}

inline bool is_odd(const U256 &n) { return n.countr_zero() == 0; }

inline Mat3 mat_pow(const Mat3 &A, U256 n)
{
    Mat3 R = IDENTITY();
    Mat3 B = A;
    while (static_cast<bool>(n))
    {
        if (is_odd(n)) R = mat_mul(R, B);
        n >>= 1;
        if (static_cast<bool>(n)) B = mat_mul(B, B);
    }
    return R;
}

inline U1024 mat_det(const Mat3 &A)
{
    const U1024 t0 = mod_sub(mod_mul(A.v[4], A.v[8]), mod_mul(A.v[5], A.v[7]));
    const U1024 t1 = mod_sub(mod_mul(A.v[3], A.v[8]), mod_mul(A.v[5], A.v[6]));
    const U1024 t2 = mod_sub(mod_mul(A.v[3], A.v[7]), mod_mul(A.v[4], A.v[6]));
    return mod_add(mod_sub(mod_mul(A.v[0], t0), mod_mul(A.v[1], t1)), mod_mul(A.v[2], t2));
}

// Обратная матрица для матриц из SL_3 (det = 1) — через адъюгату.
inline Mat3 mat_inv(const Mat3 &A)
{
    if (mat_det(A) != U1024{1})
        throw std::runtime_error("mat_inv ожидает матрицу с det=1");
    Mat3 I;
    I.v[0] = mod_sub(mod_mul(A.v[4], A.v[8]), mod_mul(A.v[5], A.v[7]));
    I.v[1] = mod_sub(mod_mul(A.v[2], A.v[7]), mod_mul(A.v[1], A.v[8]));
    I.v[2] = mod_sub(mod_mul(A.v[1], A.v[5]), mod_mul(A.v[2], A.v[4]));
    I.v[3] = mod_sub(mod_mul(A.v[5], A.v[6]), mod_mul(A.v[3], A.v[8]));
    I.v[4] = mod_sub(mod_mul(A.v[0], A.v[8]), mod_mul(A.v[2], A.v[6]));
    I.v[5] = mod_sub(mod_mul(A.v[2], A.v[3]), mod_mul(A.v[0], A.v[5]));
    I.v[6] = mod_sub(mod_mul(A.v[3], A.v[7]), mod_mul(A.v[4], A.v[6]));
    I.v[7] = mod_sub(mod_mul(A.v[1], A.v[6]), mod_mul(A.v[0], A.v[7]));
    I.v[8] = mod_sub(mod_mul(A.v[0], A.v[4]), mod_mul(A.v[1], A.v[3]));
    return I;
}

// ---------------------------------------------------------------------------
// Сериализация (big-endian, фиксированная длина элемента)
// ---------------------------------------------------------------------------
template <typename T>
inline T be_to_int(const uint8_t *d, size_t n)
{
    T v{0};
    size_t i = 0;
    while (i + 8 <= n)
    {
        uint64_t w = 0;
        for (int k = 0; k < 8; ++k) w = (w << 8) | d[i + k];
        v <<= 64;
        v |= T(w);
        i += 8;
    }
    while (i < n)
    {
        v <<= 8;
        v |= T(uint64_t(d[i]));
        ++i;
    }
    return v;
}

template <typename T>
inline std::vector<uint8_t> to_be_bytes(const T &value, size_t n)
{
    std::vector<uint8_t> out(n);
    T t = value;
    size_t i = n;
    while (i >= 8)
    {
        i -= 8;
        uint64_t w = t.to_uint64();
        for (int k = 0; k < 8; ++k) out[i + k] = uint8_t(w >> (8 * (7 - k)));
        t >>= 64;
    }
    for (size_t k = 0; k < i; ++k) out[i - 1 - k] = uint8_t((t.to_uint64() >> (8 * k)) & 0xFF);
    return out;
}

inline std::vector<uint8_t> serialize_matrix(const Mat3 &M)
{
    const size_t eb = element_bytes();
    std::vector<uint8_t> out(9 * eb);
    for (int i = 0; i < 9; ++i)
    {
        auto b = to_be_bytes(M.v[i], eb);
        std::copy(b.begin(), b.end(), out.begin() + i * eb);
    }
    return out;
}

inline Mat3 deserialize_matrix(const uint8_t *data, size_t n)
{
    const size_t eb = element_bytes();
    if (n != 9 * eb)
        throw std::runtime_error("Неверная длина сериализованной матрицы");
    Mat3 M;
    for (int i = 0; i < 9; ++i)
        M.v[i] = be_to_int<U1024>(data + i * eb, eb);
    return M;
}

inline Mat3 deserialize_matrix(const std::vector<uint8_t> &d)
{
    return deserialize_matrix(d.data(), d.size());
}

// ---------------------------------------------------------------------------
// Хэш -> скаляр в [0, q-1]
// ---------------------------------------------------------------------------
inline U256 hash_to_scalar(const std::vector<std::vector<uint8_t>> &chunks)
{
    Sha512 h;
    for (const auto &c : chunks)
        h.update(c);
    uint8_t digest[64];
    h.final(digest);
    U512 hv = be_to_int<U512>(digest, 64);
    return (hv / Q()).second; // hv mod q
}

// ---------------------------------------------------------------------------
// Случайный скаляр в [1, q-1]
// ---------------------------------------------------------------------------
inline U256 random_256()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    U256 v{0};
    for (int i = 0; i < 4; ++i)
    {
        uint64_t w = rng();
        v <<= 64;
        v |= U256(w);
    }
    return v;
}

inline U256 random_scalar()
{
    U256 m = Q() - U256{1};
    return (random_256() % m) + U256{1};
}

// ---------------------------------------------------------------------------
// Кодирование текста в матричные блоки (совместимо с Python encode_message).
//   data -> base-p "цифры" (младшая первой), впереди длина в байтах,
//   дополнение нулями до кратности 9, разбиение по 9 элементов на блок.
// ---------------------------------------------------------------------------
inline std::vector<uint8_t> to_bytes(const std::string &s)
{
    return std::vector<uint8_t>(s.begin(), s.end());
}

inline std::vector<Mat3> encode_message(const std::string &text)
{
    const std::vector<uint8_t> data(text.begin(), text.end());
    if (data.size() > MAX_MESSAGE_BYTES)
        throw std::runtime_error("Сообщение слишком длинное для этой сборки (макс. " +
                                 std::to_string(MAX_MESSAGE_BYTES) + " байт)");

    // digits: base-p представление целого из байт (младшая цифра первая)
    std::vector<U1024> digits;
    Wide m = be_to_int<Wide>(data.data(), data.size());
    if (!static_cast<bool>(m))
    {
        digits.push_back(U1024{0});
    }
    else
    {
        const WideHalf ph = widen_to<WideHalf, U1024>(P());
        while (static_cast<bool>(m))
        {
            auto qr = m / ph; // pair<Wide, WideHalf>
            digits.push_back(narrow_to<U1024>(qr.second));
            m = qr.first;
        }
    }

    std::vector<U1024> arr;
    arr.push_back(U1024{uint64_t(data.size())});
    for (const auto &d : digits) arr.push_back(d);
    while (arr.size() % 9 != 0) arr.push_back(U1024{0});

    std::vector<Mat3> blocks;
    for (size_t i = 0; i < arr.size(); i += 9)
    {
        Mat3 B;
        for (int k = 0; k < 9; ++k) B.v[k] = arr[i + k];
        blocks.push_back(B);
    }
    return blocks;
}

inline std::string decode_message(const std::vector<Mat3> &blocks)
{
    std::vector<U1024> arr;
    for (const auto &b : blocks)
        for (int k = 0; k < 9; ++k)
            arr.push_back(b.v[k]);

    if (arr.empty())
        throw std::runtime_error("Пустой набор блоков");

    const uint64_t length = arr[0].to_uint64();
    if (length > (arr.size() - 1) * element_bytes())
        throw std::runtime_error("Повреждённый блок: недопустимая длина сообщения");

    const Wide pw = widen_to<Wide, U1024>(P());
    Wide m{0};
    for (size_t i = arr.size(); i-- > 1;)
        m = m * pw + widen_to<Wide, U1024>(arr[i]);

    if (length == 0) return std::string();
    auto bytes = to_be_bytes(m, size_t(length));
    return std::string(bytes.begin(), bytes.end());
}

} // namespace mc
