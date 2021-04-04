import logging
import socket
import select
import time
from threading import Thread, Timer
from typing import Dict, List, Tuple

from . import packet


def grouper(iterable, n, fillvalue=None):
    "Collect data into fixed-length chunks or blocks"
    # grouper('ABCDEFG', 3, 'x') --> ABC DEF Gxx"
    from itertools import zip_longest
    args = [iter(iterable)] * n
    return zip_longest(*args, fillvalue=fillvalue)


class SirSocket:
    """Represents a socket for the SIR protocol."""

    BUFFER_SIZE = 8192
    TIMEOUT = 0.5

    def __init__(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.send_buf: Dict[int, bytes] = {}
        self.recv_buf: Dict[int, bytes] = {}
        self.acked_pkts = set()
        self.timers = {}
        self.send_base = 0
        self.recv_base = 0
        self.send_seqnum = 0
        self.is_running = True
        self.recv_thread = Thread(target=self._recv_loop, name="recv_thread")
        self.recv_thread.start()
        self.next_seq = 0

    def _start_timer(self, seq_num):
        timer = Timer(self.TIMEOUT, self._handle_timeout, (seq_num, ))
        timer.start()
        self.timers[self.send_seqnum] = timer

    def _handle_timeout(self, seq_num):
        if not self.is_running:
            return
        logging.debug(f"Timed out for {seq_num=}")
        try:
            self._sock.send(self.send_buf[seq_num])
        except KeyError:
            logging.debug(f"{seq_num=} not in send buffer during timeout.")
        except (ConnectionRefusedError, ConnectionAbortedError):
            logging.warning(f"Connection closed {seq_num=}.")
            self.send_buf.clear()
            self.close()
        else:
            self._start_timer(seq_num)

    def _recv_loop(self):
        while self.is_running:
            try:
                r, _, _ = select.select([self._sock], [], [], 0.1)
                if r != [self._sock]:
                    continue
                pkt, address = self._sock.recvfrom(packet.MAX_PACKET_SIZE)
                self._handle_pkt(pkt, address)
            except (ConnectionRefusedError, ConnectionAbortedError, OSError):
                logging.warning("Connection closed.")
                self.send_buf.clear()
                self.is_running = False

    def _send_ack(self, seq_num, address):
        logging.debug(f"Sending ack for {seq_num=}")
        self._sock.sendto(
            packet.Packet(seq_num, True, False, b"").into_buf(), address)

    def _handle_pkt(self, pkt, address):
        try:
            packet = packet.Packet.from_buf(pkt)
            logging.info(
                f"Received: seq_no = {packet.seq_no} data = {packet.data}")
        except AssertionError:
            logging.info("Corrupted packet received")
            return
        if packet.ack:  # packet is an ACK
            self.timers[packet.seq_no].cancel()
            del self.timers[packet.seq_no]
            logging.debug(f"Removed {self.send_base=} from send buffer.")
        elif packet.nak:
            if packet.seq_no in self.timers:
                self.timers[packet.seq_no].cancel()
            self._start_timer(packet.seq_no)
            self._sock.send(self.send_buf[packet.seq_no])

        elif len(self.recv_buf) < self.BUFFER_SIZE:
            self._send_ack(packet.seq_no, address)
            self.recv_buf[packet.seq_no] = packet.data
            logging.debug(f"Received new packet with {packet.seq_no=}.")
        else:
            logging.debug(f"Buffer Full: {packet.seq_no=}")

    def bind(self, address):
        """Bind the socket to the given interface and port."""
        self._sock.bind(address)

    def connect(self, address):
        """Establish a connection to another YARU socket."""
        self._sock.connect(address)

    def read(self) -> List[Tuple[int, bytes]]:
        """Read the data sent via datagram by the connected socket.

        Max size of data will be 65841 bytes.
        If no data is available, it returns an empty string.
        """
        items = self.recv_buf.items()
        self.recv_buf.clear()
        return sorted(items)

    def write(self, data):
        """Send the data (of type bytes) to the connected socket.

        The data should be less than 65481 bytes long.
        """
        left_space = packet.DATA_SIZE * (self.BUFFER_SIZE - len(self.send_buf))
        if left_space < len(data):
            raise ValueError(("Too much data.", left_space))
        for data in grouper(data, packet.DATA_SIZE):
            pkt = packet.Packet(self.next_seq, False, False, data).into_buf()
            self.send_buf[self.next_seq] = pkt
            self._start_timer(self.next_seq)
            self._sock.sendall(pkt)
            self.next_seq = (self.next_seq + 1) % (packet.SEQ_LIM)

    def close(self, timeout=120):
        """Clear the buffers and close the connection."""
        timed_out = False

        def flush_timeout():
            nonlocal timed_out
            timed_out = True
            self.send_buf = {}

        if timeout:
            Timer(timeout, flush_timeout).start()
        while self.send_buf:
            time.sleep(1)  # can't close if there's data to be sent
        for timer in self.timers.values():
            timer.cancel()
        self.is_running = False
        self._sock.close()
        if timed_out:
            raise TimeoutError
