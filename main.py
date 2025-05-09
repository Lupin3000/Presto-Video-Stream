from presto import Presto
from jpegdec import JPEG, JPEG_SCALE_FULL
from socket import socket
from micropython import const
import micropython
import gc


HOST: str = "192.168.4.1"
PATH: str = "/stream"
PORT: int = const(80)
BUFFER_SIZE: int = const(8192)
MAX_BUFFER_SIZE: int = const(65536)
IMAGE_START: bytes = b'\xff\xd8'
IMAGE_END: bytes = b'\xff\xd9'


def connect_to_ap() -> None:
    """
    Connects to a WLAN Access Point.

    :return: None
    """
    print("[INFO] Connecting to WLAN...")
    try:
        presto.connect()
    except (ValueError, ImportError) as e:
        print(f"[ERROR] {e}")
    print("[INFO] Connected to WLAN...")


def open_mjpeg_stream(host: str, port: int, path: str) -> socket:
    """
    Opens an MJPEG stream to the specified host and port and returns the socket
    object used for this connection. This function sends an HTTP GET request to
    initiate the stream and waits for a response to establish the connection.

    :param host: The hostname or IP address of the MJPEG server.
    :type host: str
    :param port: The port number on which to connect to the server.
    :type port: int
    :param path: The path of the HTTP GET request for initiating the MJPEG stream.
    :type path: str
    :return: The socket object representing the connection to the MJPEG stream.
    :rtype: socket
    """
    request = f"GET {path} HTTP/1.1\r\nHost: {host}\r\n\r\n"
    print("[INFO] Opening MJPEG stream...")
    s = socket()
    s.connect((host, port))
    s.send(request.encode())

    while True:
        line = s.readline()
        if line == b'\r\n' or line == b'':
            break

    print("[INFO] MJPEG stream opened!")
    return s


@micropython.viper
def find_marker(buf: ptr8, length: int, b1: int, b2: int) -> int:
    """
    Searches for a specific marker in a byte buffer. The function scans through
    the buffer to locate the first instance where two consecutive bytes match
    the given marker values. If the marker is found, its starting index is returned;
    otherwise, -1 is returned.

    :param buf: Pointer to an array of bytes where the search is performed.
    :param length: Total length of the byte buffer to search through.
    :param b1: First byte value of the marker to search for.
    :param b2: Second byte value of the marker to search for.
    :return: The starting index of the detected marker within the byte buffer,
        or -1 if the marker is not found.
    """
    for i in range(length - 1):
        if buf[i] == b1 and buf[i + 1] == b2:
            return i
    return -1


if __name__ == "__main__":
    buffer = bytearray()

    presto = Presto(full_res=False, ambient_light=False, direct_to_fb=False)
    display = presto.display
    jpeg = JPEG(display)

    BLACK = display.create_pen(0, 0, 0)
    display.set_pen(BLACK)
    display.clear()
    presto.update()

    connect_to_ap()

    stream = open_mjpeg_stream(host=HOST, port=PORT, path=PATH)

    while True:
        try:
            data = stream.read(BUFFER_SIZE)
            if not data:
                print("[INFO] Connection lost!")
                break

            buffer.extend(data)

            if len(buffer) > MAX_BUFFER_SIZE:
                print("[WARNING] Buffer overflow, clearing buffer")
                buffer = bytearray()
                gc.collect()

            while True:
                start = find_marker(buffer, len(buffer), IMAGE_START[0], IMAGE_START[1])
                end = find_marker(buffer, len(buffer), IMAGE_END[0], IMAGE_END[1])

                if start != -1 and end != -1 and end > start:
                    frame = buffer[start:end + 2]
                    buffer = buffer[end + 2:]

                    jpeg.open_RAM(frame)
                    jpeg.decode(0, 0, JPEG_SCALE_FULL, dither=True)

                    frame = None

                    presto.update()
                else:
                    break

        except OSError as e:
            print(f"[ERROR] Socket error: {e}")
            break
        except Exception as e:
            print(f"[ERROR] Unexpected error: {e}")
            break
