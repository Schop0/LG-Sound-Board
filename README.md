## Getting Started
1. Install the Arduino IDE from https://www.arduino.cc/en/software
2. Clone this LG-Sound-Board Git repository
3. Open LG-Sound-Board.ino in the Arduino IDE
4. Fix the errors about missing libraries: In the Arduino IDE, go to Sketch > Include Library > Manage Libraries ... and install the following libraries:
   1. SdFat.h: SDFat by Bill Greiman v2.2.0 (do not confuse with SdFat - Adafruit Fork)
   2. TMRpcm.h: TMRpcm by TMRh20 v1.2.3
      Patch the library source code by copying the following file:
      from LG-Sound-Board\pcmConfig.h
      to C:\Users\<user>\Documents\Arduino\libraries\TMRpcm\pcmConfig.h (overwrite it or make a backup by renaming the existing file to e.g. pcmConfig.h.orig)
   3. cppQueue.h: Queue by SMFSW v1.11.0 (do not confuse with cQueue by SMFSW)
   4. SimpleKeypad.h: SimpleKeypad by Maxime Bohrer v1.0.0
   5. ArduinoUniqueID.h: ArduinoUniqueID v1.3.0
5. Connect Arduino with USB cable to PC. It is not necessary to unplug Arduino from the SoundBoard, so leave it in for convenience.
6. In Arduino IDE, in dropdown box on top select COM1 (or COM<number> with another number), type "Nano" in the box and select "Arduino Nano" and press OK.
7. In Arduino IDE, press the arrow button on top to upload the sketch to the Arduino. This should report that it is successful.
8. Prepare the SD card by connecting it to your PC using an SD card reader, such as the purple USB stick included in the SoundBoard hardware kit.
   1. Copy file from LG-Sound-Board\SD content\index.txt to the SD card root folder.
      If desired you can customize your soundboard sounds and key mappings by modifying this file.
   2. Unzip all zip file in LG-Sound-Board\SD content\ and copy all .wav files directly in the SD card root folder
   3. In Windows Explorer, right-click on the SD card drive letter and select "Eject" to safely remove the SD card without risk of corrupting it
   4. Insert your SD card in the SD card slot on your SoundBoard
9. Restart the software on the Arduino by either pressing the button below the text "NANO" or by disconnecting and reconnecting its power.
10. Play with your SoundBoard with fresh software and SD card!

## TODOs

### Audio
- [x] Convert audio to 8 bit / 22kHz script
- [x] Configure sounds from sd-card without recompiling
- [x] Soundboard samples

### Software
- [x] Receive IR signal
- [ ] Decode IR signal
- [x] Map decoded IR signal to person/sound
- [X] Send modulated IR signal
- [x] Set one button for selecting sound bank
- [x] Make two (or more) sound banks available for TMRpcm

Nice to have
- [ ] Link sensor input to play sound

### Hardware
- [ ] Wiring A20K potmeter
- [ ] Wiring line out

Nice to have
- [ ] Extra sensor input
