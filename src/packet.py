from dataclasses import dataclass

from hashlib import md5

MAX_PACKET_SIZE: int = 64
DATA_SIZE: int = 43
CONTINUATION_PREFIX: bytes = b"\xFE\xFD"
SEQ_LIM = 2**32


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
        buf += chksm.encode('utf-8')
        anl = 0
        if self.ack: anl |= 0b10_00_00_00
        if self.nak: anl |= 0b01_00_00_00
        anl |= len(self.data)
        buf += anl.to_bytes(1, 'big', signed=False)
        buf += self.data
        buf += b'\0' * (64 - len(buf))
        return buf

    @classmethod
    def from_buf(cls, buf: bytes):
        assert len(buf) == 64
        seq_no = int.from_bytes(buf[:4], 'big', signed=False)
        chksm = buf[4:20]
        anl = buf[20]
        ack = (anl & 0b10_00_00_00) != 0
        nak = (anl & 0b01_00_00_00) != 0
        length = anl & 0b00_11_11_11
        data = buf[21:21 + length]
        assert chksm == md5(data).hexdigest().encode('utf-8')
        return Packet(seq_no, ack, nak, data)
