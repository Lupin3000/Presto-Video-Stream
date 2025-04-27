# Video Stream and Display

The code in this repository is used to stream a video from DFRobot ESP32-S3 AI Cam Module to Pimoroni Presto (_RP2350_).

## Requirements

- 2x [USB cables](https://www.dfrobot.com/product-2833.html?tracking=Mszf2HlGMStAAKkFfhNgg3QhFFchlilhR47u9vXX9o9Ko6giJYRJQdmwZjbDIvMV)
- 1x [Pimoroni Presto](https://shop.pimoroni.com/products/presto?variant=54894104019323)
- 1x [ESP32-S3 AI Cam Module](https://www.dfrobot.com/product-2899.html?tracking=Mszf2HlGMStAAKkFfhNgg3QhFFchlilhR47u9vXX9o9Ko6giJYRJQdmwZjbDIvMV)
- 1x [Arduino IDE](https://www.arduino.cc/en/software/)
- 1x [VCP Drivers](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)

## Installation

```shell
# clone repository
$ git clone ...

# change into cloned directory
$ cd Presto-Video-Stream/

# create Python virtualenv (optional)
$ python3 -m venv .venv

# enable Python virtualenv
$ source .venv/bin/activate

# update pip (optional)
(.venv) $ pip3 install -U pip

# install Python requirements
(.venv) $ pip3 install -r requirements.txt 
```

## Flash MicroPython firmware to Pimoroni Presto

1. Download the latest Presto firmware from [here](https://github.com/pimoroni/presto/releases). This code was written and tested with version 0.1.0. 
2. Ensure the latest [VCP drivers](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) are installed on your system.
3. Press the `BOOT` button and connect the Presto device (_via USB cable_) with your system.
4. Drag & Drop the firmware into a specific device directory.

## Upload Code

### DFRobot ESP32-S3 AI Cam Module

Ensure the correct [settings](https://wiki.dfrobot.com/SKU_DFR1154_ESP32_S3_AI_CAM) for Arduino IDE and install the DFRobot [LTR308 Sensor library](https://github.com/DFRobot/DFRobot_LTR308)! Use the Arduino IDE to build and upload the code (_camera.ino_) to the ESP32-S3 device.

### Pimoroni Presto

You can use `rshell` to upload the MicroPython code (_main.py_) to the Presto device.

```shell
# start rshell connection
(.venv) $ rshell -p [DEVICE PATH]

# copy file
Presto-Video-Stream> cp main.py /pyboard/main.py

# start REPL
Presto-Video-Stream> repl
```

If the camera is powered and ready, you can press keys `control` + `d` (_for soft reset_) and after few seconds the stream should be displayed on screen.

## Run Video Stream

1. Power ON (_via USB cable_) ESP32-S3 AI Cam Module
2. Power ON (_via USB cable_) Presto
3. Watch video stream
