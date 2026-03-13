import socket
import json
import struct
import threading
import time
import logging
import traceback
from typing import Optional, Callable

class SBC:
    def __init__(self, ip, port, timeout=5, buffer_size=1024, reconnect_interval=5, heartbeat_interval=10, log_level=1, name="NA", verbose=False) -> None:
        self.name = name
        self.ip = ip
        self.port = port
        self.timeout = timeout
        self.buffer_size = buffer_size
        self.reconnect_interval = reconnect_interval
        self.heartbeat_interval = heartbeat_interval
        self.log_level = log_level
        self.verbose = verbose

        self._socket: Optional[socket.socket] = None
        self._connected = False
        self._lock = threading.Lock()
        self._receiver_thread = None
        self._heartbeat_thread = None
        self._stop_event = threading.Event()

        # Setup logger
        self.logger = logging.getLogger(__name__)
        logging.basicConfig(
            level=logging.INFO,
            format="[%(levelname)s] (%(name)s) %(message)s"
        )

    #### PUBLIC FUNCTIONS
    # Connection Management
    def connect(self) -> None:
        while not self._stop_event.is_set():
            try:
                self.logger.info(f"Attempting connection to {self.name}: {self.ip}:{self.port}") 

                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(self.timeout)
                sock.connect((self.ip, self.port))
                sock.settimeout(None)

                self._socket = sock
                self._connected = True

                self.logger.info(f"INFO: Connection to {self.name} successful")

                self._start_background_threads()
                return
            except Exception as e:
                self.logger.info(f"INFO: Connection to {self.name} failed, retrying in {self.reconnect_interval} seconds.")
                time.sleep(self.reconnect_interval)

    def disconnect(self) -> None:
        self._stop_event.set()
        self._connected = False
        if self._socket:
            self._socket.close()
        self._socket = None

    def is_connected(self) -> bool:
        return self._connected
    
    # For setting parameters on SBC
    def set_values(self, keyvals: dict) -> None:
        for key, value in keyvals.items():
         self.send({"set": {key: value}})
    
    # For getting parameter values from SBC
    def get_values(self, keyvals: dict) -> None:
        for key, value in keyvals.items():
         self.send({"get": {key: value}})

    def send_command(self, cmd) -> None:
        self.send({"cmd": {"cmd": cmd}})

    #### PRIVATE FUNCTIONS
    # Message functions
    def _send_framed(self, payload: dict) -> None:
        data = json.dumps(payload).encode("utf-8")
        length_prefix = struct.pack(">I", len(data))
        with self._lock:
            self._socket.sendall(length_prefix + data)

    def _receive_framed(self) -> Optional[dict]:
        try:
            header = self._recv_exact(4)
            if not header:
                return None  # connection likely closed

            length = struct.unpack(">I", header)[0]

            # sanity check against malicious or corrupt lengths
            if length <= 0 or length > 10_000:
                self.logger.error(f"Invalid message length: {length}")
                return None

            body = self._recv_exact(length)

            try:
                return json.loads(body.decode("utf-8"))
            except json.JSONDecodeError:
                self.logger.error(f"Malformed JSON received — dropping frame")
                return None
        except socket.error:
            # Real transport failure
            self.logger.error(f"Socket Error")
            return None
        except Exception as e:
            self.logger.error(f"Unexpected receive error: {e}")
            return None
            
    def _recv_exact(self, size: int) -> bytes:
        data = b""
        while len(data) < size:
            chunk = self._socket.recv(size - len(data))
            if not chunk:
                self.logger.error(f"Unable to recieve data -Socket closed")
            data += chunk
        return data

    def send(self, payload: dict) -> None:
        if not self._connected:
            self.logger.error(f"Unable to send message - Not connected")
        try:
            self._send_framed(payload)
        except Exception:
            self._handle_disconnect()

    # Background Threads
    def _start_background_threads(self) -> None:
        # Start reciever thread to accept unprompted data from ESP32
        self._receiver_thread = threading.Thread(
            target=self._receive_loop, daemon=True
        )
        self._receiver_thread.start()

        # Start heartbeat thread to check if ESP32 is alive
        self._heartbeat_thread = threading.Thread(
            target=self._heartbeat_loop, daemon=True
        )
        self._heartbeat_thread.start()

    def _receive_loop(self) -> None:
        while self._connected and not self._stop_event.is_set():
            try:
                message = self._receive_framed()

                if message is None:
                    self.logger.warning(f"Recieved empty message")
                    continue  # just drop bad frame

                if message.get("type") == "pong":
                    continue  # heartbeat reply

            except (ConnectionError, socket.error):
                # Only reconnect on REAL transport failure
                self._handle_disconnect()
                break

    def _heartbeat_loop(self) -> None:
        while self._connected and not self._stop_event.is_set():
            try:
                self._send_framed({"type": "ping"})
            except Exception:
                self._handle_disconnect()
                return

            time.sleep(self.heartbeat_interval)

    def _handle_disconnect(self) -> None:
        if self._connected:
            self.logger.error(f"Disconnected. Reconnecting...")
        self._connected = False
        if self._socket:
            self._socket.close()
        self.connect()


if __name__ == "__main__":   
    ip = input("SBC IP address:")
    sbc = SBC(ip, 5000, verbose=True)
    try:
        print("Connecting to SBC...")
        sbc.connect()
        print("Connected. Enter JSON messages to send. Type 'quit' or Ctrl-C to exit.")

        while True:
            try:
                line = input("> ").strip()
            except EOFError:
                break

            if not line:
                continue
            if line.lower() in ("quit", "exit"):
                break

            try:
                payload = json.loads(line)
            except json.JSONDecodeError as e:
                print(f"Invalid JSON: {e}")
                continue

            try:
                sbc.send(payload)
                print("Sent")
            except Exception as e:
                print(f"Send failed: {e}")
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    finally:
        sbc.disconnect()