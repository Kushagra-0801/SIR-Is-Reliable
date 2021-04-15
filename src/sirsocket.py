import socket
import select
import time
from threading import Thread, Timer
from typing import Dict, List, Tuple

import packet
from packet import MAX_PACKET_SIZE, Packet, SEQ_LIM, FINISHER_DATA


def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    # from itertools import zip_longest
    # args = [iter(iterable)] * n
    # return (''.join(chr(i) for i in d if i is not None).encode()
    #         for d in zip_longest(*args, fillvalue=fillvalue))
    for i in range((len(iterable) + n - 1) // n):
        start = i * n
        yield iterable[start:start + n]


class SirSocket:
    """Represents a socket for the SIR protocol."""

    BUFFER_SIZE = 8192
    TIMEOUT = 0.5

    def __init__(self, address, start_seq):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.connect(address)
        self.send_buf: Dict[int, bytes] = {}
        self.recv_buf: Dict[int, bytes] = {}
        self.acked_pkts = set()
        self.timers = {}
        self.next_seq = start_seq
        # print(self._sock.getpeername())
        # print(self._sock.getblocking())

    def _start_timer(self, seq_num):
        timer = Timer(self.TIMEOUT, self._handle_timeout, (seq_num, ))
        timer.start()
        self.timers[seq_num] = timer

    def _handle_timeout(self, seq_num):
        # print(f"Timed out for {seq_num=}")
        try:
            self._sock.send(self.send_buf[seq_num])
        except KeyError:
            print(f"{seq_num=} not in send buffer during timeout.")
        except (ConnectionRefusedError, ConnectionAbortedError):
            print(f"Connection closed {seq_num=}.")
            self.send_buf.clear()
        else:
            self._start_timer(seq_num)

    def _recv(self):
        while True:
            # print("reading...")
            try:
                pack = self._sock.recv(MAX_PACKET_SIZE, socket.MSG_DONTWAIT)
                self._handle_pkt(pack)
                # print("read 1 pack")
            except BlockingIOError:
                # print("Breaking...")
                break
            except (ConnectionRefusedError, ConnectionAbortedError, OSError):
                print("Connection closed.")
                self.send_buf.clear()

    def _send_ack(self, seq_num):
        # print(f"Sending ack for {seq_num=}")
        self._sock.send(packet.Packet(seq_num, True, False, b"").into_buf())

    def _handle_pkt(self, pkt):
        try:
            packet = Packet.from_buf(pkt)
            # print(f"Received: seq_no = {packet.seq_no} data = {packet.data}")
        except AssertionError:
            print("Corrupted packet received")
            return
        if packet.ack:  # packet is an ACK
            try:
                self.timers[packet.seq_no].cancel()
                del self.timers[packet.seq_no]
            except KeyError:
                # print("Got result")
                if packet.data == FINISHER_DATA:
                    for i in self.timers:
                        i.cancel()
                    return
                try:
                    self.timers[(packet.seq_no - 1) % SEQ_LIM].cancel()
                    del self.timers[(packet.seq_no - 1) % SEQ_LIM]
                    self.recv_buf[packet.seq_no] = packet.data
                except KeyError:
                    print('Warn')
        elif packet.nak:
            if packet.seq_no in self.timers:
                self.timers[packet.seq_no].cancel()
            self._sock.send(self.send_buf[packet.seq_no])
            self._start_timer(packet.seq_no)

        elif len(self.recv_buf) < self.BUFFER_SIZE:
            self._send_ack(packet.seq_no)
            self.recv_buf[packet.seq_no] = packet.data
            # print(f"Received new packet with {packet.seq_no=}.")
        else:
            print(f"Buffer Full: {packet.seq_no=}")

    def read(self) -> List[Tuple[int, bytes]]:
        """Read the data sent via datagram by the connected socket.

        Max size of data will be 65841 bytes.
        If no data is available, it returns an empty string.
        """
        self._recv()
        items = [*self.recv_buf.items()]
        self.recv_buf.clear()
        # print(items)
        return sorted(items)

    def write(self, data):
        """Send the data (of type bytes) to the connected socket.

        The data should be less than 65481 bytes long.
        """
        left_space = packet.DATA_SIZE * (self.BUFFER_SIZE - len(self.send_buf))
        # print("in write", data)
        if left_space < len(data):
            # print("in write1")
            raise ValueError(("Too much data.", left_space))
        prev = None
        for data in grouper(data, packet.DATA_SIZE):
            if prev:
                # print("in write2")
                pkt = packet.Packet(self.next_seq, False, False,
                                    prev).into_buf()
                # print('Sending packet: ', pkt)
                self.send_buf[self.next_seq] = pkt
                self._sock.sendall(pkt)
                self._start_timer(self.next_seq)
                self.next_seq = (self.next_seq + 1) % (packet.SEQ_LIM)
            prev = data
        pkt = packet.Packet(self.next_seq, True, True, prev).into_buf()
        # print('Sending packet: ', pkt)
        self.send_buf[self.next_seq] = pkt
        self._sock.sendall(pkt)
        self._start_timer(self.next_seq)
        self.next_seq = (self.next_seq + 1) % (packet.SEQ_LIM)
