#!/usr/bin/env python3
"""Materialize and compatibility-patch the SherrrBerrr 1.0 release source.

The release source pack is split into text-safe base64 chunks so a fresh clone can
reconstruct the exact validated source tree without Git LFS or binary downloads.
Only Source/ and tools/ are materialized; release docs stay as normal Git files.

Xcode/libc++ compatibility note:
The packed 0.8-hardened source used C++20 std::atomic<std::shared_ptr<T>>. libc++'s
C++20 status still does not implement P0718, so Apple builds can reject that type.
After extraction we apply an idempotent source-level compatibility patch that uses
the long-supported atomic shared_ptr free functions instead. This changes only the
thread-safe sample pointer handoff, not the DSP algorithms or 128-parameter contract.
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


def replace_if_present(path: str, old: str, new: str) -> bool:
    file = ROOT / path
    if not file.is_file():
        raise SystemExit(f"Compatibility patch target missing: {path}")
    data = file.read_text(encoding="utf-8")
    if new in data:
        return False
    if old not in data:
        raise SystemExit(f"Compatibility patch pattern missing in {path}: {old[:80]!r}")
    file.write_text(data.replace(old, new), encoding="utf-8")
    return True


def apply_xcode_shared_ptr_compatibility_patch() -> int:
    changes = 0
    changes += replace_if_present(
        "Source/Dsp/KickEngine.h",
        "void setSampleData (std::shared_ptr<const SampleData> data) noexcept { sampleData.store (std::move (data), std::memory_order_release); }",
        "void setSampleData (std::shared_ptr<const SampleData> data) noexcept { std::atomic_store_explicit (&sampleData, std::move (data), std::memory_order_release); }",
    )
    changes += replace_if_present(
        "Source/Dsp/KickEngine.h",
        "std::atomic<std::shared_ptr<const SampleData>> sampleData;",
        "std::shared_ptr<const SampleData> sampleData;",
    )
    changes += replace_if_present(
        "Source/Dsp/KickEngine.cpp",
        "triggerSampleData = sampleData.load (std::memory_order_acquire);",
        "triggerSampleData = std::atomic_load_explicit (&sampleData, std::memory_order_acquire);",
    )
    changes += replace_if_present(
        "Source/PluginProcessor.h",
        "std::atomic<std::shared_ptr<const gsk::SampleData>> loadedSample;",
        "std::shared_ptr<const gsk::SampleData> loadedSample;",
    )
    changes += replace_if_present(
        "Source/PluginProcessor.cpp",
        "engineSample = loadedSample.load (std::memory_order_acquire);",
        "engineSample = std::atomic_load_explicit (&loadedSample, std::memory_order_acquire);",
    )
    changes += replace_if_present(
        "Source/PluginProcessor.cpp",
        "const auto publishedSample = loadedSample.load (std::memory_order_acquire);",
        "const auto publishedSample = std::atomic_load_explicit (&loadedSample, std::memory_order_acquire);",
    )
    changes += replace_if_present(
        "Source/PluginProcessor.cpp",
        "loadedSample.store (std::move (sample), std::memory_order_release);",
        "std::atomic_store_explicit (&loadedSample, std::move (sample), std::memory_order_release);",
    )
    changes += replace_if_present(
        "tools/validate_ios_1_0.py",
        'check("lock_free_sample_handoff", "loadedSample.load" in processor and "publishSample" in processor and "engineSample" in processor, "Sample changes are handed to the audio thread safely", checks)',
        'check("lock_free_sample_handoff", "atomic_load_explicit (&loadedSample" in processor and "atomic_store_explicit (&loadedSample" in processor and "publishSample" in processor and "engineSample" in processor, "Sample changes are handed to the audio thread safely with libc++-compatible atomic shared_ptr operations", checks)',
    )
    return changes


def materialize() -> None:
    if SENTINEL.exists():
        print(f"Source already materialized: {SENTINEL.relative_to(ROOT)}")
        return

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


def main() -> int:
    materialize()
    changes = apply_xcode_shared_ptr_compatibility_patch()
    print(f"Applied Xcode/libc++ shared_ptr compatibility patch: {changes} file edits")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
