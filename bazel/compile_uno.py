#!/usr/bin/env python3
"""Build an Uno application with Bazel-declared, checksum-pinned AVR inputs.

Uses the Arduino AVR platform's C/C++/link flags. No Arduino CLI, installed
Arduino packages, network access, or sketch preprocessing is required.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--core-header", type=Path, required=True)
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--stepper-header", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()

# Keep paths inside the execroot: resolving symlinks would escape the sandbox.
tools = args.compiler.absolute().parent
core = args.core_header.absolute().parent
arduino = core.parent.parent
source = args.source.absolute()
stepper_header = args.stepper_header.absolute()
out = args.out.absolute()
out.mkdir(parents=True, exist_ok=True)
wire = arduino / "libraries/Wire/src"
includes = [core, arduino / "variants/standard", wire, source.parent,
            stepper_header.parent]
common = [
    "-mmcu=atmega328p", "-DF_CPU=16000000L", "-DARDUINO=10819",
    "-DARDUINO_AVR_UNO", "-DARDUINO_ARCH_AVR", "-Os", "-flto",
    "-ffunction-sections", "-fdata-sections",
] + ["-I" + str(p) for p in includes]

def run(command):
    subprocess.run([str(p) for p in command], check=True)

with tempfile.TemporaryDirectory(prefix="book-uno-") as temporary:
    work = Path(temporary)
    sources = sorted(p for directory in (core, wire) for p in directory.rglob("*")
                     if p.suffix in (".c", ".cpp", ".S"))
    application = sorted(source.parent.glob("*.cpp"))
    core_objects, app_objects = [], []
    for index, source in enumerate(sources + application):
        obj = work / (str(index) + ".o")
        if source.suffix == ".cpp":
            tool = tools / "avr-g++"
            flags = ["-x", "c++", "-std=gnu++11", "-fno-exceptions",
                     "-fno-threadsafe-statics", "-fpermissive"]
        elif source.suffix == ".S":
            tool = tools / "avr-gcc"
            flags = ["-x", "assembler-with-cpp"]
        else:
            tool = tools / "avr-gcc"
            flags = ["-std=gnu11", "-fno-fat-lto-objects"]
        run([tool] + common + flags + ["-c", source, "-o", obj])
        (core_objects if index < len(sources) else app_objects).append(obj)
    archive = work / "core.a"
    run([tools / "avr-gcc-ar", "rcs", archive] + core_objects)
    elf = out / "book_openener.elf"
    run([tools / "avr-gcc", "-mmcu=atmega328p", "-Os", "-flto",
         "-fuse-linker-plugin", "-Wl,--gc-sections",
         "-Wl,-Map=" + str(out / "book_openener.map"), "-o", elf]
        + app_objects + [archive, "-lm"])
    run([tools / "avr-objcopy", "-O", "ihex", "-R", ".eeprom",
         elf, out / "book_openener.hex"])
    report = subprocess.check_output([str(tools / "avr-size"), "-A", str(elf)], text=True)
    sections = {}
    for line in report.splitlines():
        fields = line.split()
        if len(fields) >= 2 and fields[0].startswith("."):
            sections[fields[0]] = int(fields[1])
    flash = sum(sections.get(k, 0) for k in (".text", ".data", ".bootloader"))
    ram = sum(sections.get(k, 0) for k in (".data", ".bss", ".noinit"))
    summary = f"Uno flash: {flash}/32256 bytes; static SRAM: {ram}/2048 bytes\n"
    (out / "book_openener.size.txt").write_text(summary + report.replace(str(elf), elf.name))
    print(summary, end="")
    if flash > 32256 or ram > 2048:
        raise SystemExit("Firmware exceeds Uno memory capacity")
