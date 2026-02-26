import threading
import json
import queue
import time
import tkinter as tk
from tkinter import ttk, messagebox
from tkinter.scrolledtext import ScrolledText

from sbc_handler import SBC


class SBCGui:
    def __init__(self, master: tk.Tk):
        self.master = master
        master.title("SBC Manual Controller")

        self.sbc = None
        self.log_queue = queue.Queue()

        # Top frame: connection controls
        conn_frame = ttk.LabelFrame(master, text="Connection")
        conn_frame.grid(row=0, column=0, sticky="ew", padx=8, pady=6)

        ttk.Label(conn_frame, text="IP:").grid(row=0, column=0, sticky="w")
        self.ip_var = tk.StringVar(value="192.168.2.86")
        self.ip_entry = ttk.Entry(conn_frame, textvariable=self.ip_var, width=18)
        self.ip_entry.grid(row=0, column=1, sticky="w", padx=(4, 12))

        ttk.Label(conn_frame, text="Port:").grid(row=0, column=2, sticky="w")
        self.port_var = tk.StringVar(value="5000")
        self.port_entry = ttk.Entry(conn_frame, textvariable=self.port_var, width=8)
        self.port_entry.grid(row=0, column=3, sticky="w", padx=(4, 12))

        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.connect)
        self.connect_btn.grid(row=0, column=4, padx=4)

        self.disconnect_btn = ttk.Button(conn_frame, text="Disconnect", command=self.disconnect, state="disabled")
        self.disconnect_btn.grid(row=0, column=5, padx=4)

        # Middle frame: JSON input and send
        send_frame = ttk.LabelFrame(master, text="JSON Send")
        send_frame.grid(row=1, column=0, sticky="nsew", padx=8, pady=6)

        self.json_text = ScrolledText(send_frame, height=8)
        self.json_text.grid(row=0, column=0, columnspan=3, sticky="nsew", padx=4, pady=4)

        self.send_btn = ttk.Button(send_frame, text="Send JSON", command=self.send_json, state="disabled")
        self.send_btn.grid(row=1, column=0, sticky="w", padx=4, pady=(0,6))

        self.clear_btn = ttk.Button(send_frame, text="Clear", command=lambda: self.json_text.delete("1.0", tk.END))
        self.clear_btn.grid(row=1, column=1, sticky="w", padx=4, pady=(0,6))

        # Bottom frame: log
        log_frame = ttk.LabelFrame(master, text="Log")
        log_frame.grid(row=2, column=0, sticky="nsew", padx=8, pady=6)

        self.log_text = ScrolledText(log_frame, height=12, state="disabled")
        self.log_text.grid(row=0, column=0, sticky="nsew", padx=4, pady=4)

        # Configure grid weighting
        master.columnconfigure(0, weight=1)
        master.rowconfigure(1, weight=0)
        master.rowconfigure(2, weight=1)
        send_frame.columnconfigure(0, weight=1)
        log_frame.columnconfigure(0, weight=1)

        # Start polling queue for logs
        self.master.after(200, self._poll_queue)

    def _append_log(self, text: str) -> None:
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
        line = f"[{timestamp}] {text}\n"
        self.log_text.configure(state="normal")
        self.log_text.insert(tk.END, line)
        self.log_text.see(tk.END)
        self.log_text.configure(state="disabled")

    def _poll_queue(self) -> None:
        try:
            while True:
                item = self.log_queue.get_nowait()
                # item may be dict or string
                if isinstance(item, dict):
                    self._append_log(f"Recv: {json.dumps(item)}")
                else:
                    self._append_log(str(item))
        except queue.Empty:
            pass
        finally:
            self.master.after(200, self._poll_queue)

    def connect(self) -> None:
        ip = self.ip_var.get().strip()
        port = self.port_var.get().strip()
        try:
            port_i = int(port)
        except ValueError:
            messagebox.showerror("Invalid port", "Port must be an integer")
            return

        self.connect_btn.config(state="disabled")
        self._log_queue_put(f"INFO: Starting connection to {ip}:{port_i}")

        def worker():
            try:
                self.sbc = SBC(ip, port_i, verbose=False)
                # logger callback will be called from SBC receiver threads; push into the queue
                self.sbc.set_logger_callback(self._log_queue_put)
                self.sbc.connect()
                # connect() returns once connected
                self._log_queue_put(f"INFO: Connected to {ip}:{port_i}")
                self.master.after(0, lambda: self._on_connected())
            except Exception as e:
                self._log_queue_put(f"ERROR: Connection failed: {e}")
                self.master.after(0, lambda: self.connect_btn.config(state="normal"))

        t = threading.Thread(target=worker, daemon=True)
        t.start()

    def _on_connected(self) -> None:
        self.connect_btn.config(state="disabled")
        self.disconnect_btn.config(state="normal")
        self.send_btn.config(state="normal")

    def disconnect(self) -> None:
        if self.sbc:
            try:
                self.sbc.disconnect()
                self._log_queue_put("INFO: Disconnected")
            except Exception as e:
                self._log_queue_put(f"ERROR: During disconnect: {e}")
        self.connect_btn.config(state="normal")
        self.disconnect_btn.config(state="disabled")
        self.send_btn.config(state="disabled")

    def send_json(self) -> None:
        if not self.sbc or not self.sbc.is_connected():
            messagebox.showwarning("Not connected", "Please connect to the SBC before sending")
            return

        raw = self.json_text.get("1.0", tk.END).strip()
        if not raw:
            messagebox.showinfo("Empty", "JSON input is empty")
            return

        try:
            payload = json.loads(raw)
        except json.JSONDecodeError as e:
            messagebox.showerror("Invalid JSON", f"JSON decode error: {e}")
            return

        def send_worker():
            try:
                self.sbc.send(payload)
                self._log_queue_put(f"Sent: {json.dumps(payload)}")
            except Exception as e:
                self._log_queue_put(f"ERROR: Send failed: {e}")

        threading.Thread(target=send_worker, daemon=True).start()

    def _log_queue_put(self, item) -> None:
        # Normalize items before putting
        try:
            self.log_queue.put(item)
        except Exception:
            pass


def main():
    root = tk.Tk()
    root.geometry("800x600")
    app = SBCGui(root)
    root.mainloop()


if __name__ == "__main__":
    main()
