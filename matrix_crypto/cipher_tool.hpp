// cipher_tool.hpp — схема: шифрование (Эль-Гамаль в <T>), подпись (Шнорр)
// и аутентифицированный конверт (encrypt-then-sign). Полный порт Python cipher_tool.py.
//
// Совместимо с Python: те же параметры, сериализация, SHA-512-хэширование,
// base64 и тот же JSON-конверт {"v","ct","sig"}.
#pragma once

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "base64.hpp"
#include "matrix_crypto.hpp"

namespace mc {

// ---------------------------------------------------------------------------
// Файловые утилиты
// ---------------------------------------------------------------------------
inline std::string read_file(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        throw std::runtime_error("Не удалось открыть файл: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline void write_file(const std::string &path, const std::string &data)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        throw std::runtime_error("Не удалось записать файл: " + path);
    f.write(data.data(), std::streamsize(data.size()));
}

// Десятичные целые, идущие в JSON-тексте после ключа (для разбора ключевых файлов).
inline std::vector<std::string> ints_after(const std::string &s, const std::string &key)
{
    size_t pos = s.find(key);
    if (pos == std::string::npos)
        throw std::runtime_error("В JSON не найден ключ " + key);
    pos += key.size();
    std::vector<std::string> out;
    while (pos < s.size())
    {
        if (s[pos] >= '0' && s[pos] <= '9')
        {
            size_t st = pos;
            while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
            out.push_back(s.substr(st, pos - st));
        }
        else
        {
            ++pos;
        }
    }
    return out;
}

// Извлечь строковое значение поля JSON (для конверта "ct"/"sig").
inline std::string json_string_field(const std::string &j, const std::string &key)
{
    const std::string pat = "\"" + key + "\"";
    size_t p = j.find(pat);
    if (p == std::string::npos) return "";
    p = j.find(':', p + pat.size());
    if (p == std::string::npos) return "";
    size_t q = j.find('"', p);
    if (q == std::string::npos) return "";
    size_t r = j.find('"', q + 1);
    if (r == std::string::npos) return "";
    return j.substr(q + 1, r - q - 1);
}

// ---------------------------------------------------------------------------
// Ключи (совместимы с Python public_key.json / private_key.json)
// ---------------------------------------------------------------------------
inline Mat3 load_public_key(const std::string &path = "public_key.json")
{
    const std::string s = read_file(path);
    auto nums = ints_after(s, "\"Y\"");
    if (nums.size() < 9)
        throw std::runtime_error("Публичный ключ повреждён: нет 9 элементов Y");
    Mat3 Y;
    for (int i = 0; i < 9; ++i)
        Y.v[i] = U1024{std::string_view(nums[i])};
    return Y;
}

inline U256 load_private_key(const std::string &path = "private_key.json")
{
    const std::string s = read_file(path);
    auto nums = ints_after(s, "\"x\"");
    if (nums.empty())
        throw std::runtime_error("Закрытый ключ повреждён: нет x");
    return U256{std::string_view(nums[0])};
}

inline void save_keypair(const std::string &pub_path, const std::string &priv_path,
                         const U256 &x, const Mat3 &Y)
{
    std::ostringstream py;
    py << "[\n";
    for (int i = 0; i < 3; ++i)
    {
        py << "        [";
        for (int j = 0; j < 3; ++j)
        {
            py << Y.at(i, j).toString();
            if (j < 2) py << ", ";
        }
        py << "]";
        if (i < 2) py << ",";
        py << "\n";
    }
    py << "    ]";

    std::ostringstream os;
    os << "{\n";
    os << "    \"algorithm\": \"Matrix-ElGamal-SL3 (ElGamal + Schnorr над <T> в SL_3(F_p))\",\n";
    os << "    \"version\": 2,\n";
    os << "    \"p_bits\": " << P().bit_width() << ",\n";
    os << "    \"q_bits\": " << Q().bit_width() << ",\n";
    os << "    \"params_file\": \"matrix_params.py\",\n";
    os << "    \"base_T_sha256\": \"" << params::T_SHA256 << "\",\n";
    os << "    \"Y\": " << py.str() << "\n";
    os << "}\n";
    write_file(pub_path, os.str());

    write_file(priv_path, "{\n    \"x\": " + x.toString() + "\n}\n");
}

inline void generate_and_save_keys(const std::string &pub_path = "public_key.json",
                                   const std::string &priv_path = "private_key.json")
{
    U256 x = random_scalar();
    Mat3 Y = mat_pow(T(), x);
    save_keypair(pub_path, priv_path, x, Y);
}

// ---------------------------------------------------------------------------
// БАЗОВЫЙ ПРИМИТИВ: ШИФРОВАНИЕ / ДЕШИФРОВАНИЕ (Эль-Гамаль, без аутентификации)
//   C1 = T^r,  C2 = P * Y^r;   P = C2 * (C1^x)^(-1)
// ---------------------------------------------------------------------------
inline std::string encrypt_with_public(const Mat3 &Y, const std::string &text)
{
    const auto blocks = encode_message(text);
    std::vector<uint8_t> out;
    for (const auto &B : blocks)
    {
        U256 r = random_scalar();
        Mat3 C1 = mat_pow(T(), r);
        Mat3 Yr = mat_pow(Y, r);
        Mat3 C2 = mat_mul(B, Yr);
        auto c1 = serialize_matrix(C1);
        auto c2 = serialize_matrix(C2);
        out.insert(out.end(), c1.begin(), c1.end());
        out.insert(out.end(), c2.begin(), c2.end());
    }
    return b64_encode(out);
}

inline std::string decrypt_with_private(const U256 &x, const std::string &base64_str)
{
    const std::vector<uint8_t> raw = b64_decode(base64_str);
    const size_t eb = element_bytes();
    const size_t blk = 18 * eb; // C1 + C2
    if (raw.empty() || raw.size() % blk != 0)
        throw std::runtime_error("Некорректная длина шифртекста");

    std::vector<Mat3> blocks;
    for (size_t off = 0; off < raw.size(); off += blk)
    {
        Mat3 C1 = deserialize_matrix(raw.data() + off, 9 * eb);
        Mat3 C2 = deserialize_matrix(raw.data() + off + 9 * eb, 9 * eb);

        if (!(mat_pow(C1, Q()) == IDENTITY()))
            throw std::runtime_error("C1 не принадлежит подгруппе <T> (повреждённый шифртекст)");

        Mat3 S = mat_pow(C1, x);              // S = T^(r*x) = Y^r
        Mat3 P = mat_mul(C2, mat_inv(S));     // P = C2 * (Y^r)^(-1)
        blocks.push_back(P);
    }
    return decode_message(blocks);
}

// ---------------------------------------------------------------------------
// БАЗОВЫЙ ПРИМИТИВ: ПОДПИСЬ / ПРОВЕРКА (схема Шнорра)
//   R = T^k,  e = H(R || msg) mod q,  s = k + e*x mod q
//   Проверка: T^s == R * Y^e
// ---------------------------------------------------------------------------
inline std::string sign_with_key(const U256 &x, const std::string &text)
{
    U256 k = random_scalar();
    Mat3 R = mat_pow(T(), k);
    auto Rb = serialize_matrix(R);

    U256 e = hash_to_scalar({Rb, to_bytes(text)});

    U512 prod = U512::mult_ext(e, x);          // e*x (512 бит)
    U256 prod_mod = (prod / Q()).second;        // (e*x) mod q
    U512 sum = U512(k) + U512(prod_mod);        // k + e*x, без переполнения
    U256 s = (sum / Q()).second;

    std::vector<uint8_t> payload = Rb;
    auto sb = to_be_bytes(s, element_bytes());  // как в Python: ELEMENT_BYTES байт
    payload.insert(payload.end(), sb.begin(), sb.end());
    return b64_encode(payload);
}

inline bool verify_with_public(const Mat3 &Y, const std::string &text, const std::string &base64_sign)
{
    std::vector<uint8_t> raw;
    try
    {
        raw = b64_decode(base64_sign);
    }
    catch (...)
    {
        return false;
    }
    const size_t eb = element_bytes();
    if (raw.size() != 10 * eb)
        return false;

    Mat3 R = deserialize_matrix(raw.data(), 9 * eb);
    U1024 s_full = be_to_int<U1024>(raw.data() + 9 * eb, eb);
    if (s_full >= widen_to<U1024, U256>(Q()))   // s обязан быть < q
        return false;
    if (!(mat_pow(R, Q()) == IDENTITY()))       // R должно лежать в <T>
        return false;

    U256 s = narrow_to<U256>(s_full);
    U256 e = hash_to_scalar({serialize_matrix(R), to_bytes(text)});

    Mat3 lhs = mat_pow(T(), s);                 // T^s
    Mat3 rhs = mat_mul(R, mat_pow(Y, e));       // R * Y^e
    return lhs == rhs;
}

// ---------------------------------------------------------------------------
// АУТЕНТИФИЦИРОВАННЫЙ ФОРМАТ: encrypt-then-sign
//   sealed = base64( {"v":2,"ct":<b64 шифра>,"sig":<b64 подписи(ct)>} )
// ---------------------------------------------------------------------------
inline std::string seal_with_keys(const U256 &sender_x, const Mat3 &recipient_Y, const std::string &text)
{
    const std::string ct = encrypt_with_public(recipient_Y, text);
    const std::string sig = sign_with_key(sender_x, ct);
    const std::string json = "{\"v\":2,\"ct\":\"" + ct + "\",\"sig\":\"" + sig + "\"}";
    return b64_encode(std::vector<uint8_t>(json.begin(), json.end()));
}

inline std::string open_with_keys(const U256 &recipient_x, const Mat3 &sender_Y, const std::string &sealed)
{
    std::vector<uint8_t> raw;
    try
    {
        raw = b64_decode(sealed);
    }
    catch (...)
    {
        throw std::runtime_error("Некорректный формат аутентифицированного шифртекста");
    }
    const std::string json(raw.begin(), raw.end());
    const std::string ct = json_string_field(json, "ct");
    const std::string sig = json_string_field(json, "sig");
    if (ct.empty() || sig.empty())
        throw std::runtime_error("Некорректный формат аутентифицированного шифртекста");

    if (!verify_with_public(sender_Y, ct, sig))
        throw std::runtime_error("Аутентификация не пройдена: шифртекст изменён или подпись неверна");

    return decrypt_with_private(recipient_x, ct);
}

} // namespace mc
