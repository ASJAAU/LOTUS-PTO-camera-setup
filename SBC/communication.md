# Server -> SBC/Microcontroller communication overview
## Overview
Communication between the server and SBCs/Microcontrollers will happen with JSON serialized objects.
The JSON messages are framed with a 4-byte big-endian length prefix as a quality assurance measure. The messages should follow the format summarized below, but further detailed in the main repository.

For detailed READMEs look at the respective project folders

## Examples
### Heartbeat (`ping` / `pong`)
Request:
```{  "type": "ping"}```
Response:
```{"type": "pong", "success": true}```

### Retrieve a value (`get`)
Request (get channel 1):
```{"type": "get",  "get": ["pwm.1"]}```
Response (success):
```{"type": "response", "success": true, "value": 0}```

Request (get channel 1 and channel 2):
```{"type": "get",  "get": ["pwm.1", "pwm.2"]}```
Response (success):
```{"type": "response", "success": true, "value": 0}```
```{"type": "response", "success": true, "value": 0}```


Error (unknown key):
```{"type": "response", "success": false, "error": "unknown get key", "value": "name-of-unknown-key"}```

### Set values (`set`)
*Note:* Setter commands can have nested json objects allowing to set multiple objects at a time, the microcontroller will process them sequentially 
Request (set channel 1):
```{"type": "set", "set": {"pwm.1": 128}}```
Response (success):
```{"type": "response", "success": true, "result": "pwm.1 set"}```

Request (set all channels):
```{"type": "set", "set": { "pwm": 200 }}```
Response (success):
```{"type": "response", "success": true, "result": "all pwm channels set"}```


Request (set channel 1 and channel 2):
```{"type": "set", "set": {"pwm.1": 128, "pwm.2": 128}}```
Response:
```{"type": "response", "success": true, "result": "pwm.1 set"}```
```{"type": "response", "success": true, "result": "pwm.2 set"}```

Error (unknown set key):
```{"type": "response", "success": false, "error": "unknown set key", "value": "the-offending-key"}```


### Run specific function / command (`cmd`)
Request (run a named command):
```{"type": "cmd", "cmd": "pwmTest"}```
Response (success):
```{"type": "response", "success": true, "result": "pwmTest started"}```
Response (unknown cmd):
```{"type": "response", "success": false, "error": "unknown cmd", "value": "badcmd"}
```

## Errors
### Unknown message type
If `type` is not recognized the device responds with:
```{"type": "response", "success": false, "error": "unknown message type", "value": "<the original type>"}```
### Malformed Payload
If the incoming payload cannot be parsed the device replies with:
```{"type": "response", "success": false, "error": "invalid JSON payload"}```