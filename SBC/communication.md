# SBC -> Microcontroller communication overview
## Communication format
### Framing
- 4 bytes: big-endian unsigned integer length N
- N bytes: JSON payload (UTF-8)

### Top-level JSON keys
- `type` (string) — message type. Supported values: `ping`, `get`, `set`, `cmd`.

### Optional request keys (per `type`)
- For `get`: a string under `get` (e.g. `"pwm.1"`).
- For `set`: an object under `set` containing key/value pairs (e.g. `{"pwm.1":128}` or `{"pwm":200}`).
- For `cmd`: a string under `cmd` indicating the command to run (e.g. `"pwmTest"`).

### Common response keys
- `type` (string) — `pong` for ping replies or `response` for other replies.
- `success` (bool) — true on success, false on error.
- `result` (string) — human-friendly success message (when applicable).
- `value` — numeric return (for `get`) or echoing offending payloads on errors.
- `error` (string) — error description when `success` is false.

## Examples
### Heartbeat (`ping` / `pong`)
Request:
```
{
  "type": "ping"
}
```
Response:
```
{
  "type": "pong",
  "success": true
}
```

### Retrieve a value (`get`)
Request (get channel 1):
```
{
  "type": "get",
  "get": "pwm.1"
}
```
Response (success):
```
{
  "type": "response",
  "success": true,
  "value": 0
}
```
Response (unknown key):
```
{
  "type": "response",
  "success": false,
  "error": "unknown get key",
  "value": "name-of-unknown-key"
}
```

### Set values (`set`)
Request (set channel 1):
```
{
  "type": "set",
  "set": { "pwm.1": 128 }
}
```
Response (success):
```
{
  "type": "response",
  "success": true,
  "result": "pwm.1 set"
}
```
Request (set all channels):
```
{
  "type": "set",
  "set": { "pwm": 200 }
}
```
Response (success):
```
{
  "type": "response",
  "success": true,
  "result": "all pwm channels set"
}
```
Response (unknown set key):
```
{
  "type": "response",
  "success": false,
  "error": "unknown set key",
  "value": "led"
}
```

### Run specific function / command (`cmd`)

Request (run a named command):
```
{
  "type": "cmd",
  "cmd": "pwmTest"
}
```
Response (success):
```
{
  "type": "response",
  "success": true,
  "result": "pwmTest started"
}
```
Response (unknown cmd):
```
{
  "type": "response",
  "success": false,
  "error": "unknown cmd",
  "value": "badcmd"
}
```

## Errors
### Unknown message type
If `type` is not recognized the device responds with:
```
{
  "type": "response",
  "success": false,
  "error": "unknown message type",
  "value": "<the original type>"
}
```
### Malformed Payload
If the incoming payload cannot be parsed the device replies with:
```
{
  "type": "response",
  "success": false,
  "error": "invalid JSON payload"
}
```