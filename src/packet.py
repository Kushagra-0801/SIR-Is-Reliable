from dataclasses import dataclass

from hashlib import md5

MAX_PACKET_SIZE: int = 64
DATA_SIZE: int = 43
CONTINUATION_PREFIX: bytes = b"\xFE\xFD"
SEQ_LIM = 2**32
FINISHER_DATA = b"TEKCAP TSAL"


def hexdigest_to_bytes(digest: str) -> bytes:
    r = b""
    assert len(digest) % 2 == 0
    for i in range(len(digest) // 2):
        a = digest[2 * i]
        b = digest[2 * i + 1]
        a = int(a, 16)
        b = int(b, 16)
        c = (a << 4) | b
        r += c.to_bytes(1, 'big', signed=False)
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
        # print(self.data)
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
