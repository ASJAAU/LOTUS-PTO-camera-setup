'''
Docstring for capture.py
This script executes a series of image captures with a set of given camera parameters and a set of given lighting parameters.
The lighting and camera parameters are called by the names specified in the capture_config.yaml.
To acquire from several rigs, this script should be executed for every camera setup
'''

import argparse, yaml, logging, traceback
#Local imports
from Camera.camera_control import CameraControl
from SBC.sbc_handler import SBC

__CONFIG__ = "./config.yaml"
with open(__CONFIG__, 'r') as f:
    __CONFIG__ = yaml.safe_load(f)

parser = argparse.ArgumentParser("LOTUS-PTO Camera Rig capture")
parser.add_argument(dest='rig', help="Choice of camera to capture from", choices=__CONFIG__["setups"].keys())
parser.add_argument('-c', nargs=2, action='append', help="Provide the name of a camera config followed by the name of a lighting config [See available configs with --list_configs]")
parser.add_argument('--list_configs', action='store_true', help="List all camera and lighting configs by name")
parser.add_argument('--verbose', action='store_true', help="Enable verbose execution")
args = parser.parse_args()

#Setup logger
capture_logger = logging.getLogger(__name__)
logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] (%(name)s) %(message)s"
    )

if args.list_configs:
    capture_logger.info("#### CAMERA CONFIGS ####")
    for cam_config in list(__CONFIG__["camera_configs"].keys()):
        capture_logger.info(cam_config)
    capture_logger.info("### LIGHTING CONFIGS ###")
    for lit_config in list(__CONFIG__["light_configs"].keys()):
        capture_logger.info(lit_config)

# Get setup specific config
rig = __CONFIG__["setups"][args.rig]
capture_logger.info(f"Beginning image acquisition: {args.rig}")

#Check and report of 
if args.c is None:
    try:
        capture_logger.warning(f"No configs provided. A single image will be captured with default settings")
        #Initiate controllers
        camera_controller = CameraControl(rig["camera"]["ip"])
        micro_controller = SBC(rig["sbc"]["ip"], rig["sbc"]["port"])
        #Initiate light
        micro_controller.set_values(rig["light"])
        #Capture image
        camera_controller.snap_pic()
        #close
        micro_controller.send_command("lightOff")
        camera_controller.close()
        micro_controller.disconnect()
    except Exception:
        #Format stacktraces into a single line with | markers to indicate linebreaks
        err = traceback.format_exc().replace("\n", " | ")
        capture_logger.error(err)

if args.c is not None:
    try:
        #Initiate controllers
        camera_controller = CameraControl(rig["camera"]["ip"])
        micro_controller = SBC(rig["sbc"]["ip"], rig["sbc"]["port"])
        capture_logger.info(f"{len(args.c)} configs provided")
        for c in args.c:
            if args.verbose:
                capture_logger.info(f"Capturing an image with [{c[0]}] [{c[1]}]")
                #Initiate light
                micro_controller.set_values(rig["light"])
                #Capture image
                camera_controller.snap_pic()
                #close
        micro_controller.send_command("lightOff")
        camera_controller.close()
        micro_controller.disconnect()
    except:
        #Format stacktraces into a single line with | markers to indicate linebreaks
        err = traceback.format_exc().replace("\n", " | ")
        capture_logger.error(err)