import socket
from typing import Callable
from io import BlockingIOError

from packet import MAX_PACKET_SIZE, Packet


class SirServer:
    def __init__(self, address: str, port: int):
        self.serv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.serv_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serv_sock.bind((address, port))
        self.clients = {}

    def register_factory(self, factory: Callable):
        self.client_handler_factory = factory

    def start_loop(self):
        while True:
            try:
                packet, addr = self.serv_sock.recvfrom(MAX_PACKET_SIZE,
                                                       socket.MSG_DONTWAIT)
                packet = Packet.from_buf(packet)
                try:
                    packets = self.clients[addr](packet)
                except KeyError:
                    self.clients[addr] = self.client_handler_factory()
                    packets = self.clients[addr](packet)
                for pack in packets:
                    self.serv_sock.sendto(pack.into_buf(), addr)
            except BlockingIOError:
                for cl, addr in self.clients.items():
                    packets = cl(None)
                    for pack in packets:
                        self.serv_sock.sendto(pack.into_buf(), addr)
