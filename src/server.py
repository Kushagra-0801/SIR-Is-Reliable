from threading import Timer
from time import time
from typing import Dict, List, Union
from src.packet import DATA_SIZE, CONTINUATION_PREFIX, Packet, SEQ_LIM
from . import sirserver

import argparse

ADDRESS = ("0.0.0.0", 8080)

parser = argparse.ArgumentParser("Getter",
                                 description="Get files from a server")
parser.add_argument("server", help="IP address for the server")
parser.add_argument("port", type=int, help="Port number for the server")
args = parser.parse_args()


class Handler:
    TIMEOUT = 1

    def __init__(self):
        self.send_buf: Dict[int, bytes] = {}
        self.timers = {}
        self.timed_out = {}

    def _start_timer(self, seq_no: int):
        timer = Timer(self.TIMEOUT, self._handle_timeout, (seq_no, ))
        timer.start()
        self.timers[seq_no] = timer

    def _handle_timeout(self, seq_no: int):
        self.timed_out[seq_no] = self.send_buf[seq_no]

    def __call__(self, packet: Union[Packet, None]) -> List[Packet]:
        if packet:
            if packet.ack:
                self.timers[packet.seq_no].cancel()
                del self.timers[packet.seq_no]
            elif packet.nak:
                if packet.seq_no in self.timers:
                    self.timers[packet.seq_no].cancel()
                self.timed_out[packet.seq_no] = self.send_buf[packet.seq_no]
            else:
                pass
        to_send, self.timed_out = self.timed_out, []
        for seq_no in to_send:
            self._start_timer(seq_no)
        return [*to_send.values()]


server = sirserver.SirServer(*ADDRESS)
server.register_factory(lambda: None)
server.start_loop()
