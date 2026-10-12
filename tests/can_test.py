"""Compile and run real CAN code with portable types and host HAL stubs."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=os.environ.get("CC", "gcc"))
    parser.add_argument("--host-root", type=Path)
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error("compiler unavailable; pass --cc with a native C compiler")
    output = args.build_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["PATH"] = str(Path(compiler).parent) + os.pathsep + environment["PATH"]
    environment["TMP"] = environment["TEMP"] = environment["TMPDIR"] = str(output)
    base = [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror"]
    includes = [ROOT / "bus/dev", ROOT / "board_support/demo"]
    tests = [("dev_can_test", [ROOT / "tests/dev_can_test.c", ROOT / "bus/dev/dev_can.c"])]
    if args.host_root is not None:
        host = args.host_root.resolve()
        includes = [ROOT / "tests/can_hal_stub"] + includes + [
            host / "Core/Inc", host / "Drivers/STM32H7xx_HAL_Driver/Inc",
            host / "Drivers/CMSIS/Device/ST/STM32H7xx/Include",
            host / "Drivers/CMSIS/Include",
        ]
        base += ["-DSTM32H743xx", "-DUSE_HAL_DRIVER", "-Wno-pointer-to-int-cast",
                 "-Wno-int-to-pointer-cast"]
        tests.append(("port_hal_can_test", [ROOT / "tests/port_hal_can_test.c"]))
    for name, sources in tests:
        executable = output / (name + (".exe" if os.name == "nt" else ""))
        command = base + ["-I" + str(path) for path in includes]
        command += [str(path) for path in sources] + ["-o", str(executable)]
        subprocess.run(command, env=environment, check=True, timeout=30)
        subprocess.run([str(executable)], env=environment, check=True, timeout=30)


if __name__ == "__main__":
    main()
