import socket
import json
import struct
import threading
import time
from typing import Optional, Callable

def on_message(msg):
    # TO BE IMPLEMENTED
    # Placeholder prints mesages into the terminal
    on_message_print(msg)

def on_message_print(msg):
    print("Recieved: ", msg)    
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
        self._logger_callback: Optional[Callable[[dict], None]] = None

    #### PUBLIC FUNCTIONS
    # Connection Management
    def connect(self) -> None:
        while not self._stop_event.is_set():
            try:
                if self._logger_callback:
                    self._logger_callback(f"INFO: Attempting connection to {self.name}: {self.ip}:{self.port}") 

                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.settimeout(self.timeout)
                sock.connect((self.ip, self.port))
                sock.settimeout(None)

                self._socket = sock
                self._connected = True

                if self.verbose:
                    print(f"INFO: Connection to {self.name} successful")
                if self._logger_callback:
                    self._logger_callback(f"INFO: Connection to {self.name} successful")

                self._start_background_threads()
                return
            except Exception as e:
                if self.verbose:
                    print(f"INFO: Connection to {self.name} failed, retrying in {self.reconnect_interval} seconds.")
                if self._logger_callback:
                    self._logger_callback(f"INFO: Connection to {self.name} failed, retrying in {self.reconnect_interval} seconds.")
                time.sleep(self.reconnect_interval)

    def disconnect(self) -> None:
        self._stop_event.set()
        self._connected = False
        if self._socket:
            self._socket.close()
        self._socket = None

    def is_connected(self) -> bool:
        return self._connected
    
    # For sending commands to SBC
    def update_settings(self, settings: dict) -> None:
        self.send({"set":settings}) #PLACEHOLDER UNTIL COMMUNICATION FORMAT HAS BEEN DECIDED

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
                print(f"Invalid message length: {length}")
                return None

            body = self._recv_exact(length)

            try:
                return json.loads(body.decode("utf-8"))
            except json.JSONDecodeError:
                print("Malformed JSON received — dropping frame")
                return None
        except socket.error:
            # Real transport failure
            raise
        except Exception as e:
            print(f"Unexpected receive error: {e}")
            return None
            
    def _recv_exact(self, size: int) -> bytes:
        data = b""
        while len(data) < size:
            chunk = self._socket.recv(size - len(data))
            if not chunk:
                raise ConnectionError("Socket closed")
            data += chunk
        return data

    def send(self, payload: dict) -> None:
        if not self._connected:
            raise ConnectionError("Not connected")
        try:
            self._send_framed(payload)
        except Exception:
            self._handle_disconnect()

    def set_logger_callback(self, callback: Callable[[dict], None]) -> None:
        self._logger_callback = callback

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
                    self._logger_callback("WARNING: Recieved empty message")
                    continue  # just drop bad frame

                if message.get("type") == "pong":
                    continue  # heartbeat reply

                if self._logger_callback:
                    self._logger_callback(message)

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
            print("Disconnected. Reconnecting...")
        self._connected = False
        if self._socket:
            self._socket.close()
        self.connect()