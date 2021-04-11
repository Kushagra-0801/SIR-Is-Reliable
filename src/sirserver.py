import socket
from typing import Any, Callable, DefaultDict, Dict, List, Union
from io import BlockingIOError
from threading import Timer

from packet import MAX_PACKET_SIZE, Packet


class SirServer:
    TIMEOUT = 1
    IN_FLIGHT_BUFFER = 8192

    def __init__(self, address: str, port: int):
        self.serv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.serv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serv_sock.bind((address, port))
        self.clients: Dict[Any, Callable[[Union[Packet, None], int],
                                         List[Packet]]] = {}
        self.send_buf = {}
        self.timers = {}

    def register_factory(self,
                         factory: Callable[[],
                                           Callable[[Union[Packet, None], int],
                                                    List[Packet]]]):
        self.client_handler_factory = factory
        self.clients = DefaultDict(self.client_handler_factory)

    def _start_timer(self, seq_no: int, client_addr):
        timer = Timer(self.TIMEOUT, self._handle_timeout,
                      (seq_no, client_addr))
        timer.start()
        self.timers[client_addr][seq_no] = timer

    def _handle_timeout(self, seq_no: int, addr):
        self.serv_sock.sendto(self.send_buf[addr][seq_no].into_buf(), addr)
        self._start_timer(seq_no, addr)

    def start_loop(self):
        while True:
            try:
                packet, addr = self.serv_sock.recvfrom(MAX_PACKET_SIZE,
                                                       socket.MSG_DONTWAIT)
                packet = Packet.from_buf(packet)
                packets = []
                if packet.ack:
                    self.timers[addr][packet.seq_no].cancel()
                    del self.timers[addr][packet.seq_no]
                    del self.send_buf[addr][packet.seq_no]
                elif packet.nak:
                    if packet.seq_no in self.timers[addr]:
                        self.timers[addr][packet.seq_no].cancel()
                    packets = [self.send_buf[addr][packet.seq_no]]
                packets.extend(self.clients[addr](
                    packet, self.IN_FLIGHT_BUFFER - len(self.send_buf[addr])))
                for pack in packets:
                    self.serv_sock.sendto(pack.into_buf(), addr)
                    self._start_timer(pack.seq_no, addr)
                    self.send_buf[addr][pack.seq_no] = pack
            except BlockingIOError:
                for addr, cl in self.clients.items():
                    packets = cl(
                        None, self.IN_FLIGHT_BUFFER - len(self.send_buf[addr]))
                    for pack in packets:
                        self.serv_sock.sendto(pack.into_buf(), addr)
                        self._start_timer(pack.seq_no, addr)
                        self.send_buf[addr][pack.seq_no] = pack
