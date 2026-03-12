import threading, argparse, logging
from logging.handlers import RotatingFileHandler

from Camera.camera_control import CameraControl

# from dashboard import DashboardApp


class Main:
    def __init__(self, auto_interval=None) -> None:
        self.camera_control = CameraControl(auto_interval=auto_interval)

        self.logger = self.logging_setup()
        self.logger.info("Camera system initialized.")

        self.check_for_input()

    def logging_setup(self) -> logging.Logger:
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
            handlers=[
                RotatingFileHandler(
                    "camera_system.log", maxBytes=5 * 1024 * 1024, backupCount=2
                ),
                logging.StreamHandler(),  # Also print to terminal
            ],
        )

        logger = logging.getLogger(__name__)
        return logger

    def check_for_input(self) -> None:
        try:
            while True:
                user_input = input(
                    "Enter 'p' to take a picture, 's' to start live view, 'u' to update settings, or 'q' to quit: "
                )
                if user_input == "p":
                    self.camera_control.snap_pic(user=True)
                elif user_input == "s":
                    self.camera_control.stream()
                elif user_input == "u":
                    self.camera_control.update_settings()
                elif user_input == "q":
                    break
                else:
                    print("Invalid input. Please try again.")

        finally:
            print("Lukker kameraet")
            if hasattr(self, "camera_control") and self.camera_control.camera.IsOpen():
                self.camera_control.camera.Close()

    @staticmethod
    def run_in_thread(func, *args) -> threading.Thread:
        """General worker function to run a function in a thread"""
        thread = threading.Thread(target=func, args=args, daemon=True)
        thread.start()
        return thread


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Camera control script")

    parser.add_argument(
        "-a",
        "--auto",
        type=int,
        nargs="?",
        const=60,
        default=None,
        help="Run in auto mode. Optional: specify interval in seconds (default: 60)",
    )

    args = parser.parse_args()

    # args.auto will be None (False), or an Integer (True)
    Main(auto_interval=args.auto)
