from typing import Dict, List, Union
from packet import DATA_SIZE, CONTINUATION_PREFIX, Packet, SEQ_LIM
import sirserver
from os.path import getsize

ADDRESS = ("127.0.0.1", 8080)


def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    for i in range(0, len(iterable), n):
        yield iterable[i:i + n]


class Handler:
    def __init__(self):
        self.ack_count = set()
        self.total_packets = -1
        self.seq_no = 0
        self.file_name = b""
        self.f = None

    def __call__(self, packet: Union[Packet, None],
                 rem_buf: int) -> List[Packet]:
        packets = []
        if packet:
            if packet.ack:
                self.ack_count.add(packet.seq_no)
            elif packet.nak:
                pass
            else:
                if packet.data[:2] == CONTINUATION_PREFIX:
                    packets = [Packet.acknowledgment(packet.seq_no)]
                    self.file_name += packet.data[2:]
                else:
                    self.file_name += packet.data
                    self.seq_no = packet.seq_no
                    self._seq_no_incr()
                    try:
                        self.f = open(self.file_name, "rb")
                        s = getsize(self.file_name)
                        self.total_packets = (s + DATA_SIZE - 1) // DATA_SIZE
                        data = s.to_bytes(8, 'big', signed=True)
                    except FileNotFoundError:
                        self.total_packets = 0
                        data = (-1).to_bytes(8, 'big', signed=True)
                    packets = [Packet(self.seq_no, True, False, data)]
                    self._seq_no_incr()

        if len(self.ack_count) == self.total_packets:
            packets.append(Packet.finisher(self.seq_no))
            self.ack_count.add(SEQ_LIM)
            if self.f:
                self.f.close()
                self.f = None
        if self.f:
            to_send_now = self.f.read(DATA_SIZE * rem_buf)
            for data in grouper(to_send_now, DATA_SIZE):
                packets.append(Packet(self.seq_no, False, False, data))
                self._seq_no_incr()
        return packets

    def _seq_no_incr(self):
        self.seq_no += 1
        if self.seq_no >= SEQ_LIM:
            self.seq_no = 0


server = sirserver.SirServer(*ADDRESS)
server.register_factory(Handler)
print('Server setup at port: 8080')
server.start_loop()
