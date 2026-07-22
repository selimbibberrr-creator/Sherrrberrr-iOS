#!/usr/bin/env python3
"""Materialize the staged SherrrBerrr 1.0 release source deterministically.

The release source pack is split into text-safe base64 chunks so a fresh clone can
reconstruct the exact validated source tree without Git LFS or binary downloads.
Only Source/ and tools/ are materialized; release docs stay as normal Git files.
This script is idempotent and is the first step in every Codemagic workflow.
"""
from __future__ import annotations

import base64
import hashlib
import io
from pathlib import Path
import tarfile

ROOT = Path(__file__).resolve().parent
PACK_DIR = ROOT / ".sourcepack"
EXPECTED_SHA256 = "038eb65b3251c73ca0f8a2667f061c8b715977a6536bf874afaaae862548eed0"
SENTINEL = ROOT / "Source" / "PluginProcessor.cpp"
ALLOWED_PREFIXES = ("Source/", "tools/")


def main() -> int:
    if SENTINEL.exists():
        print(f"Source already materialized: {SENTINEL.relative_to(ROOT)}")
        return 0

    parts = sorted(PACK_DIR.glob("part*"))
    if not parts:
        raise SystemExit("No .sourcepack/part* files found")

    encoded = "".join(part.read_text(encoding="ascii").strip() for part in parts)
    payload = base64.b64decode(encoded, validate=True)
    digest = hashlib.sha256(payload).hexdigest()
    if digest != EXPECTED_SHA256:
        raise SystemExit(f"Source pack SHA-256 mismatch: {digest}")

    selected = []
    with tarfile.open(fileobj=io.BytesIO(payload), mode="r:xz") as archive:
        root = ROOT.resolve()
        for member in archive.getmembers():
            name = member.name.lstrip("./")
            target = (ROOT / name).resolve()
            if root not in target.parents and target != root:
                raise SystemExit(f"Unsafe archive member: {member.name}")
            if name.startswith(ALLOWED_PREFIXES):
                member.name = name
                selected.append(member)
        archive.extractall(ROOT, members=selected)

    if not SENTINEL.exists():
        raise SystemExit("Source pack extracted but PluginProcessor.cpp is missing")

    print(
        f"Materialized SherrrBerrr 1.0 source from {len(parts)} chunks "
        f"({digest}); extracted {len(selected)} Source/tools entries"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
