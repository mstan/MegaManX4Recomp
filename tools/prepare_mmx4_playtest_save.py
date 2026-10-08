"""Create private, original-format pre-final-stage X/Zero memory cards.

Reads palette/icon/title assets from the owner's original SLUS-00561 executable.
Writes only new files, never replaces a player's existing card. The record map
comes from original routines 8001C07C/8001C210; 8001FA24 sets story progress 5
when the eighth Maverick is cleared. This is generated playtest progress.
"""
import argparse
from functools import reduce
from pathlib import Path
import operator
import struct


def checksum(frame):
    frame[127] = reduce(operator.xor, frame[:127], 0)
    return frame


def card_image(binary, campaign):
    base = struct.unpack_from('<I', binary, 0x18)[0]
    def asset(address, size):
        offset = address - base + 0x800
        result = binary[offset:offset + size]
        if len(result) != size:
            raise ValueError('Original executable asset outside image')
        return result
    if asset(0x8001005c, 12) != b'BASLUS-00561':
        raise ValueError('Expected original US SLUS-00561 executable')
    card = bytearray([0xff] * 131072)
    header = bytearray(128)
    header[:2] = b'MC'
    card[:128] = checksum(header)
    for block in range(1, 16):
        directory = bytearray(128)
        directory[0] = 0xa0
        directory[8:10] = b'\xff\xff'
        card[block * 128:(block + 1) * 128] = checksum(directory)
    for sector in range(16, 36):
        broken = bytearray(128)
        broken[:4] = b'\xff' * 4
        broken[8:10] = b'\xff\xff'
        card[sector * 128:(sector + 1) * 128] = checksum(broken)
    card[36 * 128:63 * 128] = bytes(27 * 128)
    card[63 * 128:64 * 128] = card[:128]
    directory = bytearray(128)
    directory[0] = 0x51
    struct.pack_into('<I', directory, 4, 8192)
    directory[8:10] = b'\xff\xff'
    directory[10:22] = b'BASLUS-00561'
    card[128:256] = checksum(directory)
    saved = bytearray(8192)
    saved[:4] = b'SC\x13\x01'
    saved[4:25] = asset(0x800100a8, 21)
    saved[0x60:0x80] = asset(0x800f1fc0, 32)
    saved[0x80:0x200] = asset(0x800f1fe0, 384)
    record = bytearray(42)
    record[0] = campaign
    record[1] = 32
    record[4] = 0xff  # all eight Maverick weapons/techniques
    record[5] = 5     # native story state immediately after eighth clear
    saved[512:554] = record
    card[8192:16384] = saved
    return card


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    binary = args.exe.read_bytes()
    images = [card_image(binary, campaign) for campaign in range(2)]
    paths = [args.output / f'card{campaign + 1}.mcd' for campaign in range(2)]
    if any(path.exists() for path in paths):
        raise FileExistsError('Choose a new private output directory; cards already exist')
    args.output.mkdir(parents=True, exist_ok=True)
    for campaign, path in enumerate(paths):
        with path.open('xb') as file:
            file.write(images[campaign])
        print(f'{path}: {("X", "Zero")[campaign]}, eight Mavericks clear, final stages unlocked')


if __name__ == '__main__':
    main()
