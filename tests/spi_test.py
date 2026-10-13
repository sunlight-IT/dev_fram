"""Compile real SPI wrappers/adapters against native HAL boundary stubs."""
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
        parser.error("native compiler unavailable; pass --cc")
    output = args.build_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["PATH"] = str(Path(compiler).parent) + os.pathsep + env["PATH"]
    env["TMP"] = env["TEMP"] = env["TMPDIR"] = str(output)
    includes = [ROOT / "bus/dev", ROOT / "board_support/demo", ROOT / "common"]
    tests = [("dev_spi_test", [ROOT / "tests/dev_spi_test.c",
                              ROOT / "bus/dev/dev_spi.c",
                              ROOT / "bus/dev/dev_gpio.c"])]
    flags = ["-std=c11", "-Wall", "-Wextra", "-Werror"]
    if args.host_root is not None:
        host = args.host_root.resolve()
        includes = [ROOT / "tests/spi_hal_stub"] + includes + [
            ROOT / "port_bsp", host / "Core/Inc",
            host / "Drivers/STM32H7xx_HAL_Driver/Inc",
            host / "Drivers/CMSIS/Device/ST/STM32H7xx/Include",
            host / "Drivers/CMSIS/Include"]
        flags += ["-DSTM32H743xx", "-DUSE_HAL_DRIVER",
                  "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast"]
        adapter = ROOT / "tests/port_hal_spi_test.c"
        if adapter.exists():
            tests.append(("port_hal_spi_test", [adapter]))
        policy_test = host / "tool/tests/spi_dma_policy_test.c"
        if policy_test.exists():
            tests.append(("spi_dma_policy_test", [policy_test]))
    for name, sources in tests:
        exe = output / (name + (".exe" if os.name == "nt" else ""))
        command = [compiler] + flags + ["-I" + str(p) for p in includes]
        if name == "spi_dma_policy_test":
            command += ["-I" + str(host / "Usr/board_support/H743")]
        subprocess.run(command + [str(p) for p in sources] + ["-o", str(exe)],
                       env=env, check=True, timeout=30)
        subprocess.run([str(exe)], env=env, check=True, timeout=30)


if __name__ == "__main__":
    main()
