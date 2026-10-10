"""Catch accidental drift in the Pico/AVR sensor constants and register map."""
from pathlib import Path
import re
import os

root = (Path(os.environ["TEST_SRCDIR"]) / os.environ["TEST_WORKSPACE"]
        if "TEST_SRCDIR" in os.environ else Path(__file__).resolve().parents[2])
avr = root / "book_openener/firmware/book_openener"

def table(text, name):
    block = re.search(r"\b" + name + r"\s*\{(.*?)\};", text, re.S).group(1)
    return [int(n, 0) for n in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", block)]

pico = (root / "src/ModulinoDistance.cpp").read_text()
port = (avr / "ModulinoDistance.cpp").read_text()
for name in ("defaultConfiguration", "rangeStatuses"):
    assert table(pico, name) == table(port, name), name + " diverged from Pico driver"

def registers(path):
    return re.findall(r"Register<(?:std::)?(uint\d+_t)>\s+(\w+)\{(0x[0-9A-Fa-f]+)\}", path.read_text())

assert registers(root / "src/VL53L4CDRegisters.h") == registers(avr / "VL53L4CDRegisters.h")
print("Pico/AVR configuration, status table and register parity passed")
