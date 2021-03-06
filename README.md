# Nano Timelapser

### A little funny device for helping with timelapse videos

## Preface

I found a 16x1 LCD module from my junkbox. I was wondering what to build with that. Then I saw a 
servo. I had made some timelapse videos with an IKEA kitchen timer and SJCAM SJ4000 (which has 
quite nice timelapse functionality), but that clock rotates only counterclockwise and with 
constant speed of 360 degrees per hour. So I decided to make a little more sophisticated device 
from my junkbox parts. As mentioned before, I found a standard Hitec HS-311 servo too. I also have 
a reserve of Arduino Nanos and basic rotary encoders.

![LCD box](img/tm161aba6_box.jpg)

## The schematic

[![Schematic](img/timelapser_schema.png)](img/timelapser_schema_big.png)

## Operation

When power is plugged to the device, it first stands on the startup screen. When the switch
of the rotary encoder is pushed, it waits for the _end_ position where to turn. It can be
adjusted with the rotary encoder. Next the starting position can be set. Next is the duration
for the rotation. It works like eg. in a microwave oven, the bigger the time set is, the bigger
is the step. After that it waits for the final click and after it the rotation starts.

![Timelapser photo 1](img/timelapser1.jpg)

![Timelapser photo 2](img/timelapser2.jpg)

![Timelapser photo 3](img/timelapser3.jpg)

### Happy timelapsing.

