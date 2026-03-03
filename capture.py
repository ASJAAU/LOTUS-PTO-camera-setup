'''
Docstring for capture.py
This script executes a series of image captures with a set of given camera parameters and a set of given lighting parameters.
The lighting and camera parameters are called by the names specified in the capture_config.yaml.
To acquire from several rigs, this script should be executed for every camera setup
'''

import argparse
import yaml
#Local imports
#import Camera.camera_control as cc

__CONFIG__ = "./config.yaml"
with open(__CONFIG__, 'r') as f:
    __CONFIG__ = yaml.safe_load(f)

parser = argparse.ArgumentParser("LOTUS-PTO Camera Rig capture")
parser.add_argument(dest='rig', help="Choice of camera to capture from", choices=__CONFIG__["setups"].keys())
parser.add_argument('-c', nargs=2, action='append', help="Provide the name of a camera config followed by the name of a lighting config [See available configs with --list_configs]")
parser.add_argument('--list_configs', action='store_true', help="List all camera and lighting configs by name")
parser.add_argument('--verbose', action='store_true', help="Enable verbose execution")
args = parser.parse_args()



if args.list_configs:
    print("#### CAMERA CONFIGS ####")
    for cam_config in list(__CONFIG__["camera_configs"].keys()):
        print(cam_config)
    print("### LIGHTING CONFIGS ###")
    for lit_config in list(__CONFIG__["light_configs"].keys()):
        print(lit_config)

#Check and report of 
if args.c is None:
    print("WARNING: No configs provided. a single image will be captured with default settings") #TODO: INTEGRATE THIS WITH EXISTING LOGGING PARADIGM 

# Open Camera
#camera = cc.CameraControl()    #TODO: Implement a camera instantiation that takes an IP and returns connection state (TRUE/FALSE)
#sbc = sbc.connect()            #TODO: Implement a SBC instantiation that takes an IP and returns connection state (TRUE/FALSE)
 
if args.c is not None:
    for c in args.c:
        if args.verbose:
            print(f"INFO: Captured an image with [{c[0]}] [{c[1]}]") 
            #TODO: INTEGRATE THIS WITH EXISTING LOGGING PARADIGM 
            #Update configurations
            #sbc.update_settings(c[1]) #TODO: Implement a function that passes a dict to the SBC and updates the lighting configuration
            #camera.update_settings(c[0])
            #wait for 1 second? (to let the lights adjust)
            #camera.capture()
else:
    if args.verbose:
        print(f"INFO: Captured an image with [DEFAULT] [DEFAULT]") 
        #TODO: INTEGRATE THIS WITH EXISTING LOGGING PARADIGM
        # Close the camera
        # camera.close() #TODO: Implement a camera shutoff function (maybe we can push a setting that sleeps the sensor?)
        # sbc.close()    #TODO: Implement a function for the SBC that closses the 