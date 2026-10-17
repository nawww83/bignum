// dh_secure.hpp — аутентифицированный матричный Диффи-Хеллман (порт dh_secure.py).
//
//   A = T^a, B = T^b, общий секрет T^(ab); каждый пакет подписывается схемой Шнорра,
//   поэтому подделать пакет без закрытого ключа нельзя.
//
// Сессионный ключ = SHA-256 от канонической сериализации общего секрета
// (совместимо с hashlib.sha256 в Python).
#pragma once

#include <string>

#include "cipher_tool.hpp"

namespace mc {

inline std::string pack_matrix_to_base64(const Mat3 &M)
{
    return b64_encode(serialize_matrix(M));
}

inline Mat3 unpack_matrix_from_base64(const std::string &s)
{
    auto v = b64_decode(s);
    if (v.size() != 9 * element_bytes())
        throw std::runtime_error("Некорректный пакет: неверная длина матрицы");
    return deserialize_matrix(v);
}

inline std::string session_key_hex(const Mat3 &shared)
{
    return sha256_hex(serialize_matrix(shared));
}

// Одна сторона рукопожатия: формирует и подписывает свой пакет D = T^eph.
struct DhPacket
{
    Mat3 D;                 // T^eph
    std::string packet_b64; // base64(serialize(D))
    std::string signature;  // подпись Шнорра над packet_b64
};

inline DhPacket dh_publish(const U256 &own_x, const U256 &eph)
{
    DhPacket p;
    p.D = mat_pow(T(), eph);
    p.packet_b64 = pack_matrix_to_base64(p.D);
    p.signature = sign_with_key(own_x, p.packet_b64);
    return p;
}

// Проверяет подпись пира и вычисляет общий секрет D_peer^eph.
inline Mat3 dh_derive(const U256 &eph, const Mat3 &peer_Y,
                      const std::string &peer_packet_b64, const std::string &peer_signature)
{
    if (!verify_with_public(peer_Y, peer_packet_b64, peer_signature))
        throw std::runtime_error("Подпись пира неверна: возможна MitM-атака");
    Mat3 D_peer = unpack_matrix_from_base64(peer_packet_b64);
    return mat_pow(D_peer, eph); // T^(eph_peer * eph)
}

} // namespace mc
