import argparse

parser = argparse.ArgumentParser("Getter",
                                 description="Get files from a server")
parser.add_argument("server", help="IP address for the server")
parser.add_argument("port", type=int, help="Port number for the server")
args = parser.parse_args()
