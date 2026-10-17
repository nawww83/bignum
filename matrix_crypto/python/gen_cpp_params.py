"""
Генератор C++-заголовка с параметрами системы (p, q, T) из matrix_params.py.

Запуск (из подкаталога python/ или как `python python/gen_cpp_params.py`):
    python gen_cpp_params.py [путь_к_matrix_params.hpp]

По умолчанию пишет в C++-порт:
    \\\\wsl.localhost\\Ubuntu-24.04\\home\\novikov\\bignum\\matrix_crypto\\matrix_params.hpp

Заголовок содержит только десятичные строки — C++ парсит их через UBig::fromString,
поэтому автоматическая генерация не требует переноса big-int в C++ на этапе сборки.
"""

import sys
import hashlib
import os

import matrix_params as mp
import matrix_crypto_core as core


DEFAULT_OUT = (
    "//wsl.localhost/Ubuntu-24.04/home/novikov/bignum/matrix_crypto/matrix_params.hpp"
)


def main():
    out_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUT

    p = mp.P_MODULO
    q = mp.GROUP_ORDER_Q
    T = mp.BASE_T

    t_sha256 = hashlib.sha256(core.serialize_matrix(T)).hexdigest()

    lines = []
    lines.append("// AUTO-GENERATED from matrix_params.py — НЕ РЕДАКТИРОВАТЬ ВРУЧНУЮ.")
    lines.append("// Регенерация: python gen_cpp_params.py")
    lines.append("//")
    lines.append(f"// p : {p.bit_length()} бит (модуль поля F_p)")
    lines.append(f"// q : {q.bit_length()} бит (порядок подгруппы <T>, простое)")
    lines.append(f"// T : генератор подгруппы, order(T) = q")
    lines.append(f"// sha256(serialize(T)) = {t_sha256}")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")
    lines.append("namespace mc::params {")
    lines.append("")
    lines.append(f'inline constexpr char P_DEC[] = "{p}";')
    lines.append(f'inline constexpr char Q_DEC[] = "{q}";')
    lines.append("inline constexpr char T_DEC[9][340] = {")
    for row in T:
        for x in row:
            lines.append(f'    "{x}",')
    lines.append("};")
    lines.append("")
    lines.append(f'inline constexpr char T_SHA256[] = "{t_sha256}";')
    lines.append("")
    lines.append("} // namespace mc::params")
    lines.append("")

    text = "\n".join(lines)

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)

    print(f"[+] Записан {out_path}")
    print(f"    p: {p.bit_length()} бит, q: {q.bit_length()} бит")
    print(f"    sha256(T) = {t_sha256}")


if __name__ == "__main__":
    main()
