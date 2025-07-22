circsteer - circular analog steering

## Description
Circsteer lets you steer by moving an analog stick in circles.
It uses vJoy to pass processed inputs to the game:

	controller -> circsteer -> vjoy -> game

Circsteer also does passthrough of LT/RT (L2/R2) inputs, to help bypass
the axis smoothing that certain games apply to controllers.

The polling rate is fixed at 1000 Hz.

## Requirements
* vJoy: https://github.com/shauleiz/vJoy/releases
* an XInput-compatible controller

## Usage
* Launch circsteer before you launch your game.
* To steer, draw a circle with the left analog stick.
* To stop steering, release the stick. The virtual wheel will return to 12 o'clock.

## Setup
### cfg
Settings are read from `circsteer.cfg`, which should be in the same directory as `circsteer.exe`.  
Here is the default cfg file with a comment explaining each setting:

	range=540  # steering range (degrees lock-to-lock)
	thresh=30  # how far the stick has to be deflected to start steering (percent)
