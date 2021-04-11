from dataclasses import dataclass

from hashlib import md5

MAX_PACKET_SIZE: int = 64
DATA_SIZE: int = 43
CONTINUATION_PREFIX: bytes = b"\xFE\xFD"
SEQ_LIM = 2**32
FINISHER_DATA = b"TEKCAP TSAL"


def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    from itertools import zip_longest
    args = [iter(iterable)] * n
    return (''.join(chr(i) for i in d if i is not None).encode()
            for d in zip_longest(*args, fillvalue=fillvalue))


def hexdigest_to_bytes(digest: str) -> bytes:
    r = b""
    for a in grouper(digest.encode(), 2):
        b = 0
        if a[0] >= ord('0') and a[0] <= ord('9'):
            b = a[0] - ord('0')
        elif a[0] >= ord('a') and a[0] <= ord('f'):
            b = a[0] - ord('a')
        elif a[0] >= ord('A') and a[0] <= ord('F'):
            b = a[0] - ord('A')
        b <<= 4
        if a[1] >= ord('0') and a[1] <= ord('9'):
            b |= a[0] - ord('0')
        elif a[1] >= ord('a') and a[1] <= ord('f'):
            b |= a[1] - ord('a')
        elif a[1] >= ord('A') and a[1] <= ord('F'):
            b |= a[1] - ord('A')
        r += chr(b).encode()
    return r


@dataclass
class Packet:
    seq_no: int
    ack: bool
    nak: bool
    data: bytes

    def into_buf(self) -> bytes:
        buf = b""
        buf += self.seq_no.to_bytes(4, 'big', signed=False)
        chksm = md5(self.data).hexdigest()
        buf += hexdigest_to_bytes(chksm)
        anl = 0
        if self.ack: anl |= 0b10_00_00_00
        if self.nak: anl |= 0b01_00_00_00
        anl |= len(self.data)
        buf += anl.to_bytes(1, 'big', signed=False)
        buf += self.data
        buf += b'\0' * (64 - len(buf))
        return buf

    @staticmethod
    def from_buf(buf: bytes):
        assert len(buf) == 64
        seq_no = int.from_bytes(buf[:4], 'big', signed=False)
        chksm = buf[4:20]
        anl = buf[20]
        ack = (anl & 0b10_00_00_00) != 0
        nak = (anl & 0b01_00_00_00) != 0
        length = anl & 0b00_11_11_11
        data = buf[21:21 + length]
        assert chksm == hexdigest_to_bytes(md5(data).hexdigest())
        return Packet(seq_no, ack, nak, data)

    @staticmethod
    def finisher(seq_no: int):
        return Packet(seq_no, True, True, FINISHER_DATA)

    @staticmethod
    def acknowledgment(seq_no: int):
        return Packet(seq_no, True, False, b"")
