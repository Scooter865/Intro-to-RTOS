# Intro-to-RTOS
Examples and challenge solutions from Digikey's [Introduction to RTOS YouTube series](https://www.youtube.com/watch?v=F321087yYy4&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz).

I implemented almost everything with ESP-IDF instead of Arduino IDE. It turns out the ESP32C6 isn't fully supported by Arduino or PlatformIO as of early 2024 so I unintentionally did it the hard way.

## What to Expect
I tried to pair all of these projects down to their code files and CMakeLists files since that's what you start with when you run idf.py create-project. Note that you may need to change what's in some of the CMakeLists.txt files to compile the right files. (e.g. I changed the target .c file depending on the example)

Some reconfiguring may be required as everything is configured for my ESP32C6 (ESP32-C6-DevKit-C-1 V1.2)

I am also including some VSCode configuration files to put in your .vscode folder. Some of these are to help intellisense find things, some are for debugging which took a while to set up. See [this YouTube video](https://youtu.be/uq93H7T7cOQ?si=8YpMViW5TriGiF8z) for help on debugging.
You'll need to change the paths specified in these files so make sure to figure out where your ESP IDF is installed, where your executables are, etc.