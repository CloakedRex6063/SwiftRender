import subprocess
from pathlib import Path
import re

SLANG_BINARY = (
        Path(__file__).resolve().parent.parent
        / "examples"
        / "extern"
        / "slang"
        / "bin"
        / "slangc"
)

ROOT_DIR = Path(__file__).parent
OUT_FILE = Path(__file__).parent.parent / "inc" / "swift_shader_data.hpp"

SLANG_TARGET = "dxil"

SHADER_STAGE_MAP = {
    "compute": "cs",
    "vertex": "vs",
    "pixel": "ps",
    "geometry": "gs",
    "hull": "hs",
    "domain": "ds",
}

def detect_shader_stage(shader_path: Path) -> str:
    text = shader_path.read_text(encoding="utf-8", errors="ignore")

    match = re.search(r'\[shader\s*\(\s*"(\w+)"\s*\)\]', text)
    if not match:
        raise RuntimeError(
            f"No [shader(\"...\")] attribute found in {shader_path}"
        )

    stage_name = match.group(1).lower()

    if stage_name not in SHADER_STAGE_MAP:
        raise RuntimeError(
            f"Unsupported shader stage '{stage_name}' in {shader_path}"
        )

    return SHADER_STAGE_MAP[stage_name]

def compile_to_dxil(shader_path: Path) -> bytes:
    stage = detect_shader_stage(shader_path)
    profile = f"{stage}_6_6"
    dxil_path = shader_path.with_suffix(".dxil.tmp")

    cmd = [
        str(SLANG_BINARY),
        str(shader_path),
        "-entry", "main",
        "-target", "dxil",
        "-profile", profile,
        "-o", str(dxil_path),
    ]

    print(f"Compiling: {shader_path}")
    subprocess.run(cmd, check=True)

    data = dxil_path.read_bytes()
    dxil_path.unlink(missing_ok=True)
    return data

def main():
    arrays = []

    for file in ROOT_DIR.rglob("*"):
        if not file.is_file():
            continue

        if file.suffix == ".py":
            continue

        name = file.stem.replace("-", "_")

        try:
            bytecode = compile_to_dxil(file)
        except subprocess.CalledProcessError as e:
            print(f"Failed: {file}: {e}")
            continue

        size = len(bytecode)
        bytes_cpp = ", ".join(f"0x{b:02x}" for b in bytecode)

        arrays.append(
            f"    inline constexpr std::array<uint8_t, {size}> {name}_code = {{\n"
            f"        {bytes_cpp}\n"
            f"    }};"
        )

    OUT_FILE.write_text(
        "#pragma once\n"
        "#include \"array\"\n"
        "#include \"cstdint\"\n\n"
        "namespace Swift\n"
        "{\n"
        + "\n\n".join(arrays)
        + "\n}\n"
    )

    print(f"Written: {OUT_FILE}")

if __name__ == "__main__":
    main()
