#!/usr/bin/env python3
import argparse
import json
import sys
import zlib
from pathlib import Path


DEFAULT_BASE_URL = "https://raw.githubusercontent.com/kenhkythuat/hbee_agriconnect"
DEFAULT_BRANCH = "ota_hbee_v1"
DEFAULT_DEVICE = "wbee-stm32f103ret6"
DEFAULT_APP_ADDR = 0x08008000
DEFAULT_MAX_SIZE = 224 * 1024


def parse_int(value):
    return int(value, 0)


def load_intel_hex(path, app_addr, max_size):
    upper_addr = 0
    records = []
    max_end = app_addr
    eof_seen = False

    with path.open("r", encoding="ascii") as hex_file:
        for line_no, raw_line in enumerate(hex_file, 1):
            line = raw_line.strip()
            if not line:
                continue
            if not line.startswith(":"):
                raise ValueError(f"Line {line_no}: invalid Intel HEX record")
            try:
                count = int(line[1:3], 16)
                offset = int(line[3:7], 16)
                record_type = int(line[7:9], 16)
                data = bytes.fromhex(line[9:9 + count * 2])
                checksum = int(line[9 + count * 2:11 + count * 2], 16)
            except ValueError as exc:
                raise ValueError(f"Line {line_no}: malformed Intel HEX") from exc

            checksum_sum = count + (offset >> 8) + (offset & 0xFF)
            checksum_sum += record_type + sum(data) + checksum
            if (checksum_sum & 0xFF) != 0:
                raise ValueError(f"Line {line_no}: checksum mismatch")

            if record_type == 0x00:
                address = upper_addr + offset
                end = address + count
                if address < app_addr or end > app_addr + max_size:
                    raise ValueError(
                        f"Line {line_no}: address 0x{address:08X}..0x{end - 1:08X} "
                        f"outside app range 0x{app_addr:08X}.."
                        f"0x{app_addr + max_size - 1:08X}"
                    )
                records.append((address, data))
                max_end = max(max_end, end)
            elif record_type == 0x01:
                eof_seen = True
                break
            elif record_type == 0x04:
                if count != 2:
                    raise ValueError(f"Line {line_no}: bad extended address")
                upper_addr = int.from_bytes(data, "big") << 16
            elif record_type not in (0x02, 0x03, 0x05):
                raise ValueError(
                    f"Line {line_no}: unsupported record type 0x{record_type:02X}"
                )

    if not eof_seen or not records:
        raise ValueError("HEX has no application data or EOF record")

    image = bytearray(b"\xFF" * (max_end - app_addr))
    for address, data in records:
        start = address - app_addr
        image[start:start + len(data)] = data
    return bytes(image)


def load_firmware(path, app_addr, max_size):
    if path.suffix.lower() == ".hex":
        image = load_intel_hex(path, app_addr, max_size)
    elif path.suffix.lower() == ".bin":
        image = path.read_bytes()
    else:
        raise ValueError("Firmware must be an Intel HEX (.hex) or binary (.bin) file")

    if len(image) < 8 or len(image) > max_size:
        raise ValueError(f"Firmware size {len(image)} is outside 8..{max_size}")

    stack_pointer = int.from_bytes(image[0:4], "little")
    reset_vector = int.from_bytes(image[4:8], "little")
    reset_address = reset_vector & ~1
    if not (0x20000000 <= stack_pointer <= 0x20010000):
        raise ValueError(f"Invalid initial stack pointer 0x{stack_pointer:08X}")
    if not (reset_vector & 1):
        raise ValueError(f"Reset vector is not Thumb code: 0x{reset_vector:08X}")
    if not (app_addr <= reset_address < app_addr + max_size):
        raise ValueError(
            f"Reset vector 0x{reset_vector:08X} is outside the OTA app region"
        )
    return image


def main():
    parser = argparse.ArgumentParser(
        description="Create a contiguous OTA .bin and raw-host manifest."
    )
    parser.add_argument(
        "--firmware", type=Path,
        help="OTA application .hex or .bin linked at 0x08008000"
    )
    parser.add_argument(
        "--hex", dest="legacy_hex", type=Path,
        help="Compatibility alias for --firmware"
    )
    parser.add_argument("--version", required=True, help="For example 2.2.0")
    parser.add_argument("--branch", default=DEFAULT_BRANCH)
    parser.add_argument(
        "--base-url", default=DEFAULT_BASE_URL,
        help="Raw file base URL, without branch (GitHub or compatible host)"
    )
    parser.add_argument("--device", default=DEFAULT_DEVICE)
    parser.add_argument("--app-addr", default=DEFAULT_APP_ADDR, type=parse_int)
    parser.add_argument("--max-size", default=DEFAULT_MAX_SIZE, type=parse_int)
    parser.add_argument("--ota-dir", default=Path("ota"), type=Path)
    args = parser.parse_args()

    firmware_path = args.firmware or args.legacy_hex
    if firmware_path is None:
        parser.error("one of --firmware or --hex is required")
    if not firmware_path.exists():
        print(f"Firmware not found: {firmware_path}", file=sys.stderr)
        return 1

    try:
        image = load_firmware(firmware_path, args.app_addr, args.max_size)
    except ValueError as exc:
        print(f"OTA firmware invalid: {exc}", file=sys.stderr)
        return 1

    args.ota_dir.mkdir(parents=True, exist_ok=True)
    output_name = f"wbee_v{args.version}.bin"
    output_path = args.ota_dir / output_name
    output_path.write_bytes(image)
    crc32 = zlib.crc32(image) & 0xFFFFFFFF

    manifest = {
        "version": args.version,
        "device": args.device,
        "app_addr": f"0x{args.app_addr:08X}",
        "size": len(image),
        "crc32": f"0x{crc32:08X}",
        "bin_url": (
            f"{args.base_url.rstrip('/')}/{args.branch}/ota/{output_name}"
        ),
    }
    manifest_path = args.ota_dir / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii"
    )

    print(f"Firmware: {output_path}")
    print(f"Address:  0x{args.app_addr:08X}")
    print(f"Size:     {len(image)}")
    print(f"CRC32:    0x{crc32:08X}")
    print(f"Manifest: {manifest_path}")
    print(f"URL:      {manifest['bin_url']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
