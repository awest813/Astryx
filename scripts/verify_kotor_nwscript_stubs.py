#!/usr/bin/env python3
"""Verify KotOR NWScript function tables and report stub vs real coverage.

Fails if any function pointer is null (0). Prints per-engine counts so CI and
agents can track stub burn-down toward full behavioral parity.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
ENGINES = ("kotor", "kotor2")

PTR_RE = re.compile(
    r'\{\s*(\d+)\s*,\s*"([^"]+)"\s*,\s*(0|&Functions::(\w+))\s*\}'
)


def check_engine(engine: str) -> list[str]:
    path = REPO_ROOT / f"src/engines/{engine}/script/function_tables.h"
    content = path.read_text(encoding="utf-8")
    start = content.find("kFunctionPointers[]")
    if start < 0:
        return [f"{engine}: missing kFunctionPointers table"]

    end = content.find("kFunctionSignatures[]")
    section = content[start:end] if end > start else content[start:]

    errors: list[str] = []
    total = 0
    stub = 0
    swmg = 0
    real = 0

    for match in PTR_RE.finditer(section):
        func_id, name, ptr, method = match.group(1), match.group(2), match.group(3), match.group(4)
        total += 1
        if ptr == "0":
            errors.append(f"{engine}: ID {func_id} {name} has null pointer")
            continue
        if method == "stubFunction":
            stub += 1
        elif method == "stubSWMGFunction":
            swmg += 1
        else:
            real += 1

    incomplete = stub + swmg
    pct_real = (100.0 * real / total) if total else 0.0
    print(
        f"{engine}: total={total} real={real} generic_stub={stub} "
        f"swmg_stub={swmg} incomplete={incomplete} "
        f"behavioral={pct_real:.1f}% null={len(errors)}"
    )
    return errors


def main() -> int:
    all_errors: list[str] = []
    for engine in ENGINES:
        all_errors.extend(check_engine(engine))

    if all_errors:
        print("NWScript stub verification failed:", file=sys.stderr)
        for err in all_errors:
            print(f"  {err}", file=sys.stderr)
        return 1

    print("KotOR NWScript stub verification passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
