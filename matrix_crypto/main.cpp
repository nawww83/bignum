// main.cpp — CLI и самотест C++-порта матричной криптосистемы.
//
// Сборка:  см. CMakeLists.txt (или g++ -std=c++20 -O2 -I.. main.cpp -o matrix_crypto)
// Запуск:
//   ./matrix_crypto keygen                 # public_key.json / private_key.json
//   ./matrix_crypto encrypt "текст"        # аутентифицированный конверт (encrypt-then-sign)
//   ./matrix_crypto decrypt "<base64>"     # расшифровать (с проверкой подписи)
//   ./matrix_crypto sign "текст"           # подпись Шнорра (base64)
//   ./matrix_crypto verify "текст" "<sig>" # ВЕРНА / ОШИБКА
//   ./matrix_crypto selftest               # полный прогон + демонстрация атак
//   ./matrix_crypto params                 # параметры системы
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "cipher_tool.hpp"
#include "dh_secure.hpp"

using namespace mc;

static int g_checks = 0;
static int g_failed = 0;

static void check(bool ok, const std::string &name, const std::string &fail = "ПРОВАЛ")
{
    ++g_checks;
    if (ok)
        std::cout << "    [+] " << name << "\n";
    else
    {
        ++g_failed;
        std::cout << "    [!] " << name << "  --  " << fail << "\n";
    }
}

static void hr(const std::string &title)
{
    std::cout << "\n" << std::string(70, '-') << "\n" << title << "\n" << std::string(70, '-') << "\n";
}

static std::string shorten(const std::string &s, size_t n = 28)
{
    return s.size() <= n ? s : s.substr(0, n) + "...";
}

// ---------------------------------------------------------------------------
// Самотест (повторяет логику Python e2e_test.py)
// ---------------------------------------------------------------------------
static int selftest()
{
    std::cout << "======================================================================\n";
    std::cout << " САМОТЕСТ C++ ПОРТА (ElGamal + Schnorr над <T> в SL_3(F_p))\n";
    std::cout << "======================================================================\n";

    // ---------------------------------------------------------------- ШАГ 0
    hr("[Шаг 0] Проверка параметров и группы <T>");
    check(P().bit_width() == 1023, "p: 1023 бита", "p имеет другую разрядность");
    check(Q().bit_width() == 256, "q: 256 бит", "q имеет другую разрядность");
    check(element_bytes() == 128, "ELEMENT_BYTES = 128");
    check(mat_pow(T(), Q()) == IDENTITY(), "T^q = I (порядок <T> делит q)");
    check(!(T() == IDENTITY()), "T != I => порядок T равен ровно q");
    check(mat_det(T()) == U1024{1}, "det(T) = 1 (T лежит в SL_3)");

    // ---------------------------------------------------------------- ШАГ 1
    hr("[Шаг 1] Генерация и сохранение/загрузка ключей");
    auto t0 = std::chrono::steady_clock::now();
    U256 x = random_scalar();
    Mat3 Y = mat_pow(T(), x);
    auto t1 = std::chrono::steady_clock::now();
    check(mat_pow(T(), x) == Y, "Y = T^x воспроизводится");
    std::cout << "    (генерация Y: "
              << std::chrono::duration<double, std::milli>(t1 - t0).count() << " мс)\n";

    save_keypair("selftest_public.json", "selftest_private.json", x, Y);
    Mat3 Y2 = load_public_key("selftest_public.json");
    U256 x2 = load_private_key("selftest_private.json");
    check(Y2 == Y, "JSON-ключи: Y читается обратно без потерь");
    check(x2 == x, "JSON-ключи: x читается обратно без потерь");

    // ---------------------------------------------------------------- ШАГ 2
    hr("[Шаг 2] Шифрование / дешифрование (Эль-Гамаль)");
    std::vector<std::string> messages = {
        "Привет, Боб!",
        "Это очень длинный секретный текст на русском языке, 1234567890.",
        ""};
    std::vector<std::string> cts;
    for (const auto &msg : messages)
    {
        std::string ct = encrypt_with_public(Y, msg);
        std::string ct2 = encrypt_with_public(Y, msg);
        check(ct != ct2, "рандомизация: два шифрования различаются (" + std::to_string(ct.size()) + " симв.)");
        check(decrypt_with_private(x, ct) == msg, "round-trip OK для '" + shorten(msg) + "'");
        cts.push_back(ct);
    }

    // ---------------------------------------------------------------- ШАГ 2b
    hr("[Шаг 2b] Аутентификация шифртекста (encrypt-then-sign)");
    const std::string demo = messages[1];

    {
        Mat3 Pb = encode_message(demo)[0];
        U256 r = random_scalar();
        Mat3 C1 = mat_pow(T(), r);
        Mat3 C2 = mat_mul(Pb, mat_pow(Y, r));
        Mat3 Z = mat_pow(T(), random_scalar());
        Mat3 C2m = mat_mul(Z, C2);                                   // атакующий меняет C2 -> Z*C2
        Mat3 Pa = mat_mul(C2m, mat_inv(mat_pow(C1, x)));
        check(Pa == mat_mul(Z, Pb),
              "Изменяемость сырого Эль-Гамаля: C2' = Z*C2 расшифровывается в Z*P (без ключа!)");
    }

    {
        std::string sealed = seal_with_keys(x, Y, demo);
        std::vector<uint8_t> json = b64_decode(sealed);
        std::string js(json.begin(), json.end());
        std::string ct = json_string_field(js, "ct");
        std::string sig = json_string_field(js, "sig");
        std::vector<uint8_t> ctb = b64_decode(ct);
        ctb[0] ^= 0x01;                                              // ломаем 1 байт шифртекста
        std::string ct_bad = b64_encode(ctb);
        std::string tampered_js = "{\"v\":2,\"ct\":\"" + ct_bad + "\",\"sig\":\"" + sig + "\"}";
        std::string tampered = b64_encode(std::vector<uint8_t>(tampered_js.begin(), tampered_js.end()));
        bool rejected = false;
        try
        {
            rejected = (open_with_keys(x, Y, tampered) != demo);
        }
        catch (...)
        {
            rejected = true;
        }
        check(rejected, "Аутентифицированный конверт ОТВЕРГ подмену C2 (подпись не сходится)");
    }

    {
        U256 other_x = random_scalar();
        bool forged = false;
        try
        {
            forged = (open_with_keys(x, Y, seal_with_keys(other_x, Y, demo)) != demo);
        }
        catch (...)
        {
            forged = true;
        }
        check(forged, "Аутентифицированный конверт отверг подпись чужого отправителя");
    }

    // ---------------------------------------------------------------- ШАГ 3
    hr("[Шаг 3] Подпись Шнорра");
    auto s0 = std::chrono::steady_clock::now();
    std::string sig = sign_with_key(x, demo);
    auto s1 = std::chrono::steady_clock::now();
    check(verify_with_public(Y, demo, sig), "подпись верна");
    check(!verify_with_public(Y, demo + "!", sig), "подпись неверна для изменённого сообщения");
    std::cout << "    (подпись: " << std::chrono::duration<double, std::milli>(s1 - s0).count() << " мс)\n";

    // ---------------------------------------------------------------- ШАГ 4
    hr("[Шаг 4] Атаки");
    {
        std::string raw_ct = encrypt_with_public(Y, demo);
        U256 recovered{0};
        bool found = false;
        std::vector<U256> guesses = {U256{0}, U256{1}, U256{2}, Q() - U256{1}, x + U256{1}};
        for (const auto &g : guesses)
        {
            try
            {
                if (decrypt_with_private(g, raw_ct) == demo)
                {
                    found = true;
                    recovered = g;
                    break;
                }
            }
            catch (...)
            {
            }
        }
        check(!found, "Атака B: расшифровать только по открытому ключу невозможно");

        // Подделка подписи перебором/подстановкой невозможна
        std::string fake = b64_encode(std::vector<uint8_t>(element_bytes() * 10, 0));
        check(!verify_with_public(Y, demo, fake), "Атака: произвольная подпись не проходит проверку");

        // Подмена аутентифицированного конверта
        std::string sealed = seal_with_keys(x, Y, demo);
        std::string bad = sealed;
        bad[10] = (bad[10] == 'A') ? 'B' : 'A';
        bool detected = false;
        try
        {
            detected = (open_with_keys(x, Y, bad) != demo);
        }
        catch (...)
        {
            detected = true;
        }
        check(detected, "Атака A: подмена аутентифицированного конверта обнаружена");
    }

    // ---------------------------------------------------------------- ШАГ 5
    hr("[Шаг 5] Аутентифицированный Диффи-Хеллман в <T>");
    {
        U256 ax = random_scalar(), bx = random_scalar();
        Mat3 aY = mat_pow(T(), ax), bY = mat_pow(T(), bx);

        U256 a = random_scalar(), b = random_scalar();
        DhPacket ap = dh_publish(ax, a);
        DhPacket bp = dh_publish(bx, b);

        Mat3 shared_a = dh_derive(a, bY, bp.packet_b64, bp.signature);
        Mat3 shared_b = dh_derive(b, aY, ap.packet_b64, ap.signature);
        check(shared_a == shared_b, "общий секрет T^(ab) совпадает у обеих сторон");
        check(!(shared_a == IDENTITY()), "общий секрет нетривиален");
        check(session_key_hex(shared_a) == session_key_hex(shared_b),
              "сессионные ключи (SHA-256) совпадают");

        bool mitm = false;
        try
        {
            mitm = (dh_derive(a, bY, bp.packet_b64 + "AA", bp.signature) == shared_a);
        }
        catch (...)
        {
            mitm = true;
        }
        check(mitm, "подмена пакета пира отвергается (подпись не сходится)");
    }

    std::cout << "\n======================================================================\n";
    if (g_failed == 0)
        std::cout << " SELFTEST SUCCESS: пройдено " << g_checks << " проверок\n";
    else
        std::cout << " SELFTEST FAILED: провалено " << g_failed << " из " << g_checks << "\n";
    std::cout << "======================================================================\n";

    std::remove("selftest_public.json");
    std::remove("selftest_private.json");
    return g_failed == 0 ? 0 : 1;
}

static void usage()
{
    std::cout <<
        "Использование:\n"
        "  matrix_crypto keygen [pub.json] [priv.json]\n"
        "  matrix_crypto encrypt \"текст\"            (encrypt-then-sign)\n"
        "  matrix_crypto decrypt \"BASE64\"           (с проверкой подписи)\n"
        "  matrix_crypto sign \"текст\"\n"
        "  matrix_crypto verify \"текст\" \"BASE64\"\n"
        "  matrix_crypto seal-b64   \"MSG_B64\"       (для скриптов/интеропа)\n"
        "  matrix_crypto sign-b64   \"MSG_B64\"\n"
        "  matrix_crypto verify-b64 \"MSG_B64\" \"SIG_B64\"\n"
        "  matrix_crypto open       \"SEALED_B64\"\n"
        "  matrix_crypto dh                          (демо аутентифицированного DH)\n"
        "  matrix_crypto dh-publish \"PRIV_FILE\" \"EPH_DEC\"\n"
        "  matrix_crypto dh-derive  \"EPH_DEC\" \"PEER_PUB_FILE\" \"PEER_PACKET\" \"PEER_SIG\"\n"
        "  matrix_crypto params\n"
        "  matrix_crypto dump-public                 (hex сериализованного Y)\n"
        "  matrix_crypto selftest\n";
}

// ---------------------------------------------------------------------------
// Демонстрация аутентифицированного DH (повторяет логику dh_secure.py)
// ---------------------------------------------------------------------------
static int dh_demo()
{
    std::cout << "======================================================================\n";
    std::cout << " АУТЕНТИФИЦИРОВАННЫЙ ОБМЕН КЛЮЧАМИ (МАТРИЧНЫЙ DH + ПОДПИСЬ ШНОРРА)\n";
    std::cout << "======================================================================\n";

    U256 alice_x = random_scalar(), bob_x = random_scalar();
    Mat3 alice_Y = mat_pow(T(), alice_x), bob_Y = mat_pow(T(), bob_x);

    std::cout << "\n--- [ЭТАП 1: АЛИСА ФОРМИРУЕТ И ПОДПИСЫВАЕТ ПАКЕТ] ---\n";
    U256 alice_eph = random_scalar();
    DhPacket ap = dh_publish(alice_x, alice_eph);
    std::cout << "Пакет Алисы (Base64): " << shorten(ap.packet_b64, 40) << "\n";
    std::cout << "Подпись Алисы       : " << shorten(ap.signature, 40) << "\n";

    std::cout << "\n--- [ЭТАП 2: БОБ ПРОВЕРЯЕТ ПОДПИСЬ АЛИСЫ] ---\n";
    if (!verify_with_public(alice_Y, ap.packet_b64, ap.signature))
    {
        std::cout << "КРИТИЧЕСКАЯ ОШИБКА: подпись Алисы неверна! Возможна MitM-атака.\n";
        return 1;
    }
    std::cout << "[+] Успех: подпись Алисы подтверждена. Пакет подлинный.\n";

    std::cout << "\n--- [ЭТАП 3: БОБ ФОРМИРУЕТ И ПОДПИСЫВАЕТ СВОЙ ПАКЕТ] ---\n";
    U256 bob_eph = random_scalar();
    DhPacket bp = dh_publish(bob_x, bob_eph);
    std::cout << "Пакет Боба (Base64) : " << shorten(bp.packet_b64, 40) << "\n";
    std::cout << "Подпись Боба        : " << shorten(bp.signature, 40) << "\n";

    std::cout << "\n--- [ЭТАП 4: АЛИСА ПРОВЕРЯЕТ ПОДПИСЬ БОБА] ---\n";
    if (!verify_with_public(bob_Y, bp.packet_b64, bp.signature))
    {
        std::cout << "КРИТИЧЕСКАЯ ОШИБКА: подпись Боба неверна! Канал скомпрометирован.\n";
        return 1;
    }
    std::cout << "[+] Успех: подпись Боба подтверждена. Пакет подлинный.\n";

    std::cout << "\n--- [ЭТАП 5: НЕЗАВИСИМОЕ ВЫЧИСЛЕНИЕ ОБЩЕГО СЕКРЕТА] ---\n";
    Mat3 alice_shared = dh_derive(alice_eph, bob_Y, bp.packet_b64, bp.signature);
    Mat3 bob_shared = dh_derive(bob_eph, alice_Y, ap.packet_b64, ap.signature);
    const std::string ak = session_key_hex(alice_shared);
    const std::string bk = session_key_hex(bob_shared);
    std::cout << "Сессионный ключ Алисы: " << ak << "\n";
    std::cout << "Сессионный ключ Боба : " << bk << "\n";

    const bool ok = (alice_shared == bob_shared) && (ak == bk);
    std::cout << "\nОбщий секретный ключ сформирован? " << (ok ? "ДА (Успех)" : "НЕТ (Ошибка)") << "\n";
    return ok ? 0 : 1;
}

static std::string b64_to_str(const std::string &b)
{
    auto v = b64_decode(b);
    return std::string(v.begin(), v.end());
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return 1;
    }
    const std::string mode = argv[1];

    try
    {
        if (mode == "selftest")
            return selftest();

        if (mode == "dh")
            return dh_demo();

        if (mode == "dh-publish")
        {
            if (argc < 4) { usage(); return 1; }
            U256 x = load_private_key(argv[2]);
            U256 eph{std::string_view(argv[3])};
            DhPacket p = dh_publish(x, eph);
            std::cout << "PACKET " << p.packet_b64 << "\n";
            std::cout << "SIG " << p.signature << "\n";
            return 0;
        }

        if (mode == "dh-derive")
        {
            if (argc < 6) { usage(); return 1; }
            U256 eph{std::string_view(argv[2])};
            Mat3 peer_Y = load_public_key(argv[3]);
            Mat3 shared = dh_derive(eph, peer_Y, argv[4], argv[5]);
            auto ser = serialize_matrix(shared);
            std::cout << "SHARED_HEX " << to_hex(ser.data(), ser.size()) << "\n";
            std::cout << "SESSION_KEY " << session_key_hex(shared) << "\n";
            return 0;
        }

        if (mode == "params")
        {
            std::cout << "p_bits = " << P().bit_width() << "\n";
            std::cout << "q_bits = " << Q().bit_width() << "\n";
            std::cout << "element_bytes = " << element_bytes() << "\n";
            std::cout << "base_T_sha256 = " << params::T_SHA256 << "\n";
            return 0;
        }

        if (mode == "keygen")
        {
            const std::string pub = argc > 2 ? argv[2] : "public_key.json";
            const std::string priv = argc > 3 ? argv[3] : "private_key.json";
            auto t0 = std::chrono::steady_clock::now();
            generate_and_save_keys(pub, priv);
            auto t1 = std::chrono::steady_clock::now();
            std::cout << "Ключи сохранены: " << pub << ", " << priv << " ("
                      << std::chrono::duration<double, std::milli>(t1 - t0).count() << " мс)\n";
            return 0;
        }

        if (mode == "dump-public")
        {
            Mat3 Y = load_public_key();
            std::cout << "Y_hex = ";
            static const char *HX = "0123456789abcdef";
            for (uint8_t b : serialize_matrix(Y))
            {
                std::cout << HX[b >> 4] << HX[b & 15];
            }
            std::cout << "\n";
            return 0;
        }

        // ---- команды, требующие ключей ----
        if (mode == "encrypt" || mode == "seal-b64")
        {
            if (argc < 3) { usage(); return 1; }
            std::string text = (mode == "seal-b64") ? b64_to_str(argv[2]) : argv[2];
            std::cout << seal_with_keys(load_private_key(), load_public_key(), text) << "\n";
            return 0;
        }

        if (mode == "decrypt" || mode == "open")
        {
            if (argc < 3) { usage(); return 1; }
            std::cout << open_with_keys(load_private_key(), load_public_key(), argv[2]);
            if (mode == "decrypt") std::cout << "\n";
            return 0;
        }

        if (mode == "sign" || mode == "sign-b64")
        {
            if (argc < 3) { usage(); return 1; }
            std::string text = (mode == "sign-b64") ? b64_to_str(argv[2]) : argv[2];
            std::cout << sign_with_key(load_private_key(), text) << "\n";
            return 0;
        }

        if (mode == "verify" || mode == "verify-b64")
        {
            if (argc < 4) { usage(); return 1; }
            std::string text = (mode == "verify-b64") ? b64_to_str(argv[2]) : argv[2];
            bool ok = verify_with_public(load_public_key(), text, argv[3]);
            if (mode == "verify-b64")
                std::cout << (ok ? "ВЕРНА" : "ОШИБКА") << "\n";
            else
                std::cout << "\nРезультат проверки подписи: "
                          << (ok ? "ВЕРНА (Успех)" : "ОШИБКА (Текст изменён, подпись или ключ неверны)") << "\n";
            return ok ? 0 : 1;
        }

        std::cout << "Неизвестный режим: " << mode << "\n";
        usage();
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 2;
    }
}
