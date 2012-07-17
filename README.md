Atra-Hasis Steganography Tool
=============================

Purpose
-------

Atra-Hasis is a program written in **C** that allows users to embed encrypted plaintext into a **PNG** image file. Currently, the program only encodes uncompressed PNG files without alpha channels.

Use
---

Atra-Hasis can be compiled from the command line on any system with access to gcc. To compile, run "make" from the command line while in the folder containing all the source files. This creates an executable named "AtraHasis."

To encode data into an image, 
	./AtraHasis [input_image.png] [output_image.png] [text_file.txt]

To decode data from an image, 
	./AtraHasis [encoded_image.png]



Included Files
______________

* `Makefile`
* `AtraHasis.c, .h` -- main program
* `Geshtu.c, .h` -- for image encoding
* `Jarasandha.c, .h` -- for text encryption
* test pngs
* 'text_example.txt' -- short example text file
* 'example2.txt' -- unicode text examples

Known Issues
____________

* Infinite loop on Linux
* Black line at the bottom of and unviewable thumbnail for encoded png files

Future Development
__________________

* Cleanup of known bugs
* User password input to generate encryption key

Credits
_______

© Jordan Kodner and Anand Sundaram, University of Pennsylvania School of Engineering and Applied Sciences, all rights reserved, 2012.