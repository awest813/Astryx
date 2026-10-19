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
ID_RE = re.compile(r'\{\s*(\d+)\s*,')


def parse_id_list(content: str, marker: str) -> list[int]:
    start = content.find(marker)
    if start < 0:
        return []
    # End at the next top-level array or EOF-ish close of this array.
    chunk = content[start:]
    end = chunk.find("\n};")
    section = chunk[: end + 3] if end >= 0 else chunk
    return [int(m.group(1)) for m in ID_RE.finditer(section)]


def check_table_alignment(engine: str) -> list[str]:
    """Pointers/signatures/defaults must share the same ID sequence (registerFunctions asserts this)."""
    path = REPO_ROOT / f"src/engines/{engine}/script/function_tables.h"
    content = path.read_text(encoding="utf-8")
    errors: list[str] = []

    pointers = parse_id_list(content, "kFunctionPointers[]")
    signatures = parse_id_list(content, "kFunctionSignatures[]")
    defaults = parse_id_list(content, "kFunctionDefaults[]")

    if not pointers or not signatures or not defaults:
        return [f"{engine}: failed to parse NWScript tables for alignment check"]

    if not (len(pointers) == len(signatures) == len(defaults)):
        errors.append(
            f"{engine}: table length mismatch pointers={len(pointers)} "
            f"signatures={len(signatures)} defaults={len(defaults)}"
        )

    for i, (p, s, d) in enumerate(zip(pointers, signatures, defaults)):
        if not (p == s == d):
            errors.append(
                f"{engine}: ID misalignment at index {i}: "
                f"pointer={p} signature={s} default={d}"
            )
            break

    if engine == "kotor" and 220 not in pointers:
        errors.append(f"{engine}: missing ApplyEffectToObject (id 220)")

    if engine == "kotor":
        # Retail KotOR I IDs from xoreos-tools game_kotor.h
        required = {
            315: "GetSkillRank",
            317: "GetLastAttackType",
            320: "GetIsInCombat",
            324: "SetLocked",
        }
        names = {}
        for match in PTR_RE.finditer(
            content[content.find("kFunctionPointers[]") : content.find("kFunctionSignatures[]")]
        ):
            names[int(match.group(1))] = match.group(2)
        for func_id, name in required.items():
            if names.get(func_id) != name:
                errors.append(
                    f"{engine}: id {func_id} expected {name}, got {names.get(func_id)!r}"
                )

    return errors


DEFAULT_DECL_RE = re.compile(
    r"static const Aurora::NWScript::Variable\s+(\w+)\s*\((.*)\);"
)
SIG_RE = re.compile(r"\{\s*(\d+)\s*,\s*kType\w+\s*,\s*\{([^}]*)\}\s*\}")
DEF_RE = re.compile(r"\{\s*(\d+)\s*,\s*\{([^}]*)\}\s*\}")


def default_type(init: str) -> str | None:
    init = init.strip()
    if init.startswith("&"):
        return "kTypeObject"
    if "UString" in init:
        return "kTypeString"
    if "int32_t" in init:
        return "kTypeInt"
    if "," in init:
        return "kTypeVector"
    if re.fullmatch(r"\(?\s*-?[\d.]+f\s*\)?", init):
        return "kTypeFloat"
    return None


def check_default_types(engine: str) -> list[str]:
    """Defaults are right-aligned onto the parameters; their types must match.

    FunctionContext::setDefaults() asserts this, so a mismatch aborts debug
    builds while the engine registers its NWScript functions.
    """
    path = REPO_ROOT / f"src/engines/{engine}/script/function_tables.h"
    content = path.read_text(encoding="utf-8")
    errors: list[str] = []

    types: dict[str, str] = {}
    for match in DEFAULT_DECL_RE.finditer(content):
        kind = default_type(match.group(2))
        if kind is None:
            errors.append(f"{engine}: cannot infer type of default {match.group(1)}")
        else:
            types[match.group(1)] = kind

    sig_start = content.find("kFunctionSignatures[]")
    def_start = content.find("kFunctionDefaults[]")
    signatures = {
        int(m.group(1)): [t.strip() for t in m.group(2).split(",") if t.strip()]
        for m in SIG_RE.finditer(content[sig_start:def_start])
    }

    for match in DEF_RE.finditer(content[def_start:]):
        func_id = int(match.group(1))
        names = [d.strip().lstrip("&") for d in match.group(2).split(",") if d.strip()]
        params = signatures.get(func_id, [])
        if len(names) > len(params):
            errors.append(f"{engine}: id {func_id} has more defaults than parameters")
            continue
        for name, param in zip(names, params[len(params) - len(names):]):
            if types.get(name) != param:
                errors.append(
                    f"{engine}: id {func_id} default {name} ({types.get(name)}) "
                    f"does not match parameter type {param}"
                )

    return errors


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
    errors.extend(check_table_alignment(engine))
    errors.extend(check_default_types(engine))
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
