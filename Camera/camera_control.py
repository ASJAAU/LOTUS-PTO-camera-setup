from pypylon import pylon
import cv2, time, threading, os, logging, yaml

from Camera.config_loader import config_loader


class CameraControl:
    def __init__(self, config=None, ip=None, interval=None, logger=None) -> None:
        #Instansiate logger or accept passed logger
        if logger:
            self.logger = logger
        else:
            self.logger = logging.getLogger(__name__)

        #Mark initiation
        self.logger.info(f"Camera Controller {ip}")

        # Get camera
        if ip is None: #If not IP specified get first available device
            self.camera = pylon.InstantCamera(pylon.TlFactory.GetInstance().CreateFirstDevice())
        else:
            device_info = pylon.DeviceInfo()
            device_info.SetPropertyValue("IpAddress", ip)
            tl_factory = pylon.TlFactory.GetInstance()
            device = tl_factory.CreateFirstDevice(device_info)
            self.camera = pylon.InstantCamera(device)
            if self.camera is None:
                self.logger.error(f"No camera found at ip: {ip}")
                self.close()
                return None

        # Open cammera
        self.camera.Open()
        self.camera_mutex = threading.Lock()

        # Setup config
        if config:
            self.update_settings(config)
        else:
            self.logger.warning(f"Camera config not provided, using factory camera configuration")

        for folder in ["./Captured_images"]:
            os.makedirs(folder, exist_ok=True)

        if interval is not None:
            self.run_in_thread(self.auto_pic_snapper, interval)

    def snap_pic(self, user: bool = False) -> None:
        """
        Captures a single frame from the Basler camera and saves it to disk.
        If called by the user, prompts for saving or viewing the image.

        Args:
            user (bool): If True, saves to './User_images'. If False, saves to './Captured_images'.

        Raises:
            TimeoutException: If the camera fails to return a frame within 5000ms.
        """

        try:
            with self.camera_mutex:
                self.camera.StartGrabbingMax(1)

                grabResult = self.camera.RetrieveResult(
                    5000, pylon.TimeoutHandling_ThrowException
                )

            if grabResult.GrabSucceeded():
                img = grabResult.Array

                if user:
                    while True:
                        user_input = input(
                            "Press s to save the image, v to view or q to quit: "
                        )

                        if user_input == "s":
                            timestamp = time.strftime("%Y%m%d-%H%M%S")
                            filename = f"image_{timestamp}.png"
                            full_path = os.path.join("./Captured_images", filename)
                            cv2.imwrite(full_path, img)
                            self.logger.info(f"User saved image as {full_path}")

                        elif user_input == "v":
                            cv2.imshow("Captured Image", img)
                            cv2.waitKey(0)
                            cv2.destroyAllWindows()

                        elif user_input == "q":
                            break

                        else:
                            print("Invalid input. Please try again.")

                else:
                    timestamp = time.strftime("%Y%m%d-%H%M%S")
                    filename = f"image_{timestamp}.png"
                    full_path = os.path.join("./Captured_images", filename)
                    cv2.imwrite(full_path, img)
                    self.logger.info(f"Auto saved image as {full_path}")

            else:
                self.logger.error("Failed to grab image.")

            grabResult.Release()

        except Exception as e:
            self.logger.error(f"Error capturing image: {e}")
            self.try_reconnect()

    def stream(self) -> None:
        """
        Starts a live video stream from the Basler camera using OpenCV.

        Raises:
            TimeoutException: If the camera fails to return a frame within 5000ms.
        """

        try:
            with self.camera_mutex:
                self.camera.StartGrabbing(pylon.GrabStrategy_LatestImageOnly)

                # Converter til OpenCV format (fra Basler format til BGR/RGB) (Skal kun bruges ved bayer rg 8)
                #converter = pylon.ImageFormatConverter()
                #converter.OutputPixelFormat = pylon.PixelType_BGR8packed
                #converter.OutputBitAlignment = pylon.OutputBitAlignment_MsbAligned

                print("Live view kører... Tryk på 'q' for at afslutte.")
                self.logger.info("Live view started.")

                prev_time = time.time()

                while self.camera.IsGrabbing():
                    grabResult = self.camera.RetrieveResult(
                        5000, pylon.TimeoutHandling_ThrowException
                    )

                    if grabResult.GrabSucceeded():
                        # Konverter billedet til et format OpenCV kan forstå (numpy array)
                        #image = converter.Convert(grabResult)
                        #frame = image.GetArray()

                        frame = grabResult.Array

                        # Calculate FPS
                        current_time = time.time()
                        fps_actual = 1 / (current_time - prev_time)
                        prev_time = current_time

                        # Overlay FPS text on the image
                        cv2.putText(
                            frame,
                            f"FPS: {fps_actual:.1f}",
                            (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            1,
                            (0, 255, 0),
                            2,
                        )

                        # Vis billedet i et vindue
                        cv2.imshow("Basler ace 2 Live View", frame)

                        # Stop hvis brugeren trykker på 'q'
                        if cv2.waitKey(1) & 0xFF == ord("q"):
                            self.logger.info("Live view stopped by user.")
                            break
                    else:
                        self.logger.error("Failed to grab image (stream).")

                    grabResult.Release()

                # Ryd op
                self.camera.StopGrabbing()

            cv2.destroyAllWindows()
        
        except Exception as e:
            self.logger.error(f"Error during live stream: {e}")
            self.try_reconnect()

    def auto_pic_snapper(self, interval: int) -> None:
        """
        Automatically takes pictures at specified intervals.

        Args:
            interval (int): Time in seconds between each picture.

        Raises:
            TimeoutException: If the camera fails to return a frame within 5000ms.
        """

        self.logger.info(f"Auto picture snapper started with interval {interval} seconds.")
        
        while True:
            self.snap_pic(user=False)
            self.logger.info(f"Captured image at: {time.strftime("%Y%m%d-%H%M%S")}")
            time.sleep(interval)

    def update_settings(self, config) -> None:
        """Loads camera settings from config file."""

        try:
            self.last_config = config
            self.load_config(config)
            self.logger.info("Camera settings updated.")
        
        except Exception as e:
            self.logger.error(f"Error updating settings: {e}")
            self.try_reconnect()
    
    def try_reconnect(self):
        """Attempts to re-open the camera if lost."""

        try:
            self.camera = pylon.InstantCamera(
            pylon.TlFactory.GetInstance().CreateFirstDevice()
        )
            self.camera.Close()
            self.camera.Open()
            self.update_settings(self.last_config)
            self.logger.info("Camera reconnected successfully.")

        except Exception as e:
            self.logger.error(f"Reconnection failed: {e}")

    def close(self):
        if hasattr(self, "camera") and self.camera.IsOpen():
            camera_control.camera.Close()
        self.logger.info("Stopped")

    def load_config(self, config):
        # Convert string to dict
        if type(config) == str:
            with open(config, "r") as file:
                self.config = yaml.safe_load(file)["DEFAULT"]["camera_config"]
        elif type(config) == dict: #Assume correct dict and continue
            self.config = config
        else:
            self.logger.error(f"Inappropriate config type ('{type(config)}')")
            raise TypeError(f"Inappropriate config type ('{type(config)}'), must be of type 'str' or 'dict'")
            

        image_settings = self.config["image_settings"]
        video_settings = self.config["video_settings"]
        lighting_settings = self.config["lighting_settings"]
        auto_settings = self.config["auto_settings"]

        # Image settings
        self.camera.Width.Value = image_settings["width"]
        self.camera.Height.Value = image_settings["height"]
        self.camera.OffsetX.Value = image_settings["offset_x"]
        self.camera.OffsetY.Value = image_settings["offset_y"]

        self.camera.ExposureTime.Value = lighting_settings["exposure_time"]
        self.camera.Gain.Value = lighting_settings["gain"]

        # Video settings
        enable_acquisition = video_settings["enable_acquisition"].lower()
        if enable_acquisition == "on":
            self.camera.AcquisitionFrameRateEnable.Value = True
            self.camera.AcquisitionFrameRate.Value = video_settings["acquisition_fps"]
        elif enable_acquisition == "off":
            self.camera.AcquisitionFrameRateEnable.Value = False
        else:
            raise ValueError("Invalid enable_acquisition value in config.yaml")

        # Auto settings
        self.camera.AutoTargetBrightness.Value = auto_settings["auto_brightness_target"]

        auto_exposure = auto_settings["auto_exposure"].lower()
        if auto_exposure == "off":
            self.camera.ExposureAuto.Value = "Off"
        elif auto_exposure == "once":
            self.camera.ExposureAuto.Value = "Once"
        elif auto_exposure == "continuous":
            self.camera.ExposureAuto.Value = "Continuous"
        else:
            raise ValueError("Invalid auto_exposure value in config.yaml")

        self.camera.AutoExposureTimeLowerLimit.Value = auto_settings["auto_exposure_lower_limit"]
        self.camera.AutoExposureTimeUpperLimit.Value = auto_settings["auto_exposure_upper_limit"]

        auto_function = auto_settings["auto_function"].lower()
        if auto_function == "min_gain":
            self.camera.AutoFunctionProfile.Value = "MinimizeGain"
        elif auto_function == "min_exposure":
            self.camera.AutoFunctionProfile.Value = "MinimizeExposureTime"
        else:
            raise ValueError("Invalid auto_function value in config.yaml")

        auto_gain = auto_settings["auto_gain"].lower()
        if auto_gain == "off":
            self.camera.GainAuto.Value = "Off"
        elif auto_gain == "once":
            self.camera.GainAuto.Value = "Once"
        elif auto_gain == "continuous":
            self.camera.GainAuto.Value = "Continuous"
        else:
            raise ValueError("Invalid auto_gain value in config.yaml")

        self.camera.AutoGainLowerLimit.Value = auto_settings["auto_gain_lower_limit"]
        self.camera.AutoGainUpperLimit.Value = auto_settings["auto_gain_upper_limit"]

        # Pixel format
        pixel_format_mapping = {
            "mono8": "Mono8",
            "mono10": "Mono10",
            "mono10p": "Mono10p",
            "mono12p": "Mono12p",
            "rgb8": "RGB8",
            "brg8": "BGR8",
            "ycbcr422": "YCbCr422_8",
            "bayer_gr8": "BayerGR8",
            "bayer_rg8": "BayerRG8",
            "bayer_gb8": "BayerGB8",
            "bayer_bg8": "BayerBG8",
            "bayer_gr10": "BayerGR10",
            "bayer_rg10": "BayerRG10",
            "bayer_gb10": "BayerGB10",
            "bayer_bg10": "BayerBG10",
            "bayer_gr10p": "BayerGR10p",
            "bayer_rg10p": "BayerRG10p",
            "bayer_gb10p": "BayerGB10p",
            "bayer_bg10p": "BayerBG10p",
            "bayer_gr12": "BayerGR12",
            "bayer_rg12": "BayerRG12",
            "bayer_gb12": "BayerGB12",
            "bayer_bg12": "BayerBG12",
            "bayer_gr12p": "BayerGR12p",
            "bayer_rg12p": "BayerRG12p",
            "bayer_gb12p": "BayerGB12p",
            "bayer_bg12p": "BayerBG12p",
        }

        pixel_format = image_settings["pixel_format"].lower()
        if pixel_format in pixel_format_mapping:
            camera.PixelFormat.Value = pixel_format_mapping[pixel_format]
        else:
            raise ValueError("Invalid pixel_format value in config.yaml")

    @staticmethod
    def run_in_thread(func, *args) -> threading.Thread:
        """General worker function to run a function in a thread"""

        thread = threading.Thread(target=func, args=args, daemon=True)
        thread.start()
        return thread

if __name__ == "__main__":
    camera_control = CameraControl("./config.yaml")

    try:
        while True:
            user_input = input(
                "Enter 'p' to take a picture, 's' to start live view, 'u' to update settings, or 'q' to quit: "
            )
            if user_input == "p":
                camera_control.snap_pic(user=True)
            elif user_input == "s":
                camera_control.stream()
            elif user_input == "u":
                camera_control.update_settings("./config.yaml")
            elif user_input == "q":
                break
            else:
                print("Invalid input. Please try again.")

    finally:
        camera_control.close()
