import yaml


def config_loader(camera, config="config.yaml") -> None:
    """
    Assumes a configuration dict, if string assume yaml and load. applies settings to the camera.

    Args:
        camera: The Basler camera object to configure.
        config: Dict or string path to the camera configuration
    """
    # Convert string to dict
    if type(config) == str:
        with open(config, "r") as file:
            config = yaml.safe_load(file)["DEFAULT"]["camera_config"]
    #elif type(config) == dict: #Assume correct dict and continue
    #    continue
    else:
        print(f"ERROR: Inappropriate config type ('{type(config)}')") #TODO: INTEGRATE THIS WITH EXISTING LOGGING PARADIGM 
        raise TypeError(f"Inappropriate config type ('{type(config)}'), must be of type 'str' or 'dict'")
        

    image_settings = config["image_settings"]
    video_settings = config["video_settings"]
    lighting_settings = config["lighting_settings"]
    auto_settings = config["auto_settings"]

    # Image settings
    camera.Width.Value = image_settings["width"]
    camera.Height.Value = image_settings["height"]
    camera.OffsetX.Value = image_settings["offset_x"]
    camera.OffsetY.Value = image_settings["offset_y"]

    camera.ExposureTime.Value = lighting_settings["exposure_time"]
    camera.Gain.Value = lighting_settings["gain"]

    # Video settings
    enable_acquisition = video_settings["enable_acquisition"].lower()
    if enable_acquisition == "on":
        camera.AcquisitionFrameRateEnable.Value = True
        camera.AcquisitionFrameRate.Value = video_settings["acquisition_fps"]
    elif enable_acquisition == "off":
        camera.AcquisitionFrameRateEnable.Value = False
    else:
        raise ValueError("Invalid enable_acquisition value in config.yaml")

    # Auto settings
    camera.AutoTargetBrightness.Value = auto_settings["auto_brightness_target"]

    auto_exposure = auto_settings["auto_exposure"].lower()
    if auto_exposure == "off":
        camera.ExposureAuto.Value = "Off"
    elif auto_exposure == "once":
        camera.ExposureAuto.Value = "Once"
    elif auto_exposure == "continuous":
        camera.ExposureAuto.Value = "Continuous"
    else:
        raise ValueError("Invalid auto_exposure value in config.yaml")

    camera.AutoExposureTimeLowerLimit.Value = auto_settings["auto_exposure_lower_limit"]
    camera.AutoExposureTimeUpperLimit.Value = auto_settings["auto_exposure_upper_limit"]

    auto_function = auto_settings["auto_function"].lower()
    if auto_function == "min_gain":
        camera.AutoFunctionProfile.Value = "MinimizeGain"
    elif auto_function == "min_exposure":
        camera.AutoFunctionProfile.Value = "MinimizeExposureTime"
    else:
        raise ValueError("Invalid auto_function value in config.yaml")

    auto_gain = auto_settings["auto_gain"].lower()
    if auto_gain == "off":
        camera.GainAuto.Value = "Off"
    elif auto_gain == "once":
        camera.GainAuto.Value = "Once"
    elif auto_gain == "continuous":
        camera.GainAuto.Value = "Continuous"
    else:
        raise ValueError("Invalid auto_gain value in config.yaml")

    camera.AutoGainLowerLimit.Value = auto_settings["auto_gain_lower_limit"]
    camera.AutoGainUpperLimit.Value = auto_settings["auto_gain_upper_limit"]

    # Pixel format
    pixel_format_mapping = {
        "mono8": "Mono8",
        "mono10": "Mono10",
        "mono10p": "Mono10p",
        "mono12p": "Mono12p",
        "rgb8": "RGB8",
        "bgr8": "BGR8",
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
