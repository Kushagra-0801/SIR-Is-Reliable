from packet import DATA_SIZE, CONTINUATION_PREFIX, SEQ_LIM
import sirsocket

from random import randrange
import argparse

parser = argparse.ArgumentParser("Getter",
                                 description="Get files from a server")
parser.add_argument("server", help="IP address of the server")
parser.add_argument("port", type=int, help="Port number of the server")
parser.add_argument("server_file_path",
                    metavar="S_PATH",
                    help="Path of file on server")
parser.add_argument("client_file_path",
                    metavar="C_PATH",
                    help="Path where the file should be copied to")
args = parser.parse_args()

print(f'Args: {args}')

start_no = randrange(0, SEQ_LIM // 2)
socket = sirsocket.SirSocket((args.server, args.port), start_no)

print('Created socket')

if len(path := args.server_file_path) > DATA_SIZE:
    parts = sirsocket.grouper(path.encode(),
                              DATA_SIZE - len(CONTINUATION_PREFIX))
    prev = next(parts)
    data = b""
    for p in parts:
        data += CONTINUATION_PREFIX
        data += prev
        prev = p
    data += prev
else:
    data = path.encode()
print('Sending data:', data)
socket.write(data)

while not (file_size := socket.read()):
    pass

print('Got replies')
print(file_size)
exit(0)
file_data = file_size[1:]
file_size = file_size[0][1]

file_size = int.from_bytes(file_size[:8], 'big', signed=True)

if file_size < 0:
    raise Exception("File does not exist on the server")

with open(args.client_file_path, "w+b") as f:
    print(file_size)
    # exit(0)
    f.write(b"\0" * file_size)
    f.seek(0)
    while True:
        pkts = file_data
        # ASSUME: data is always max in packet except maybe last packet
        for (seq_no, data) in pkts:
            rel_seq = (seq_no - start_no) % SEQ_LIM
            target = rel_seq * DATA_SIZE
            f.seek(target)
            f.write(data)
