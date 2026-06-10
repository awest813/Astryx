#!/usr/bin/env python3
"""Verify KotOR NWScript function tables have no null pointers."""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
ENGINES = ("kotor", "kotor2")


def check_engine(engine: str) -> list[str]:
    path = REPO_ROOT / f"src/engines/{engine}/script/function_tables.h"
    content = path.read_text(encoding="utf-8")
    start = content.find("kFunctionPointers[]")
    if start < 0:
        return [f"{engine}: missing kFunctionPointers table"]

    section = content[start : content.find("kFunctionSignatures[]")]
    errors: list[str] = []

    for match in re.finditer(
        r'\{\s*(\d+)\s*,\s*"([^"]+)"\s*,\s*(0|&Functions::\w+)\s*\}', section
    ):
        func_id, name, ptr = match.groups()
        if ptr == "0":
            errors.append(f"{engine}: ID {func_id} {name} has null pointer")

    null_count = len(errors)
    stub_count = section.count("&Functions::stubFunction")
    swmg_count = section.count("&Functions::stubSWMGFunction")
    print(
        f"{engine}: null={null_count} generic_stub={stub_count} swmg_stub={swmg_count}"
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
