Atra-Hasis Steganography Tool
=============================

Purpose
-------

Atra-Hasis is a program written in **C** that allows users to embed encrypted plaintext into a **PNG** image file. Currently, the program only encodes uncompressed PNG files without alpha channels.

Use
---

Atra-Hasis can be compiled from the command line on any system with access to gcc. To compile, run "make" from the command line while in the folder containing all the source files. This creates an executable named "AtraHasis."

To access help, use the `-h` flag:

	./AtraHasis -h

To encode data into an image:

	./AtraHasis [input_image.png] [output_image.png] [text_file.txt]
User will be prompted for a password.


To decode data from an image: 

	./AtraHasis [encoded_image.png]
User will be prompted for a password.

When decoding encrypted data, if an incorrect password is used, the output will be unintelligible. In order to retrieve the original text, the correct password must be used. Thus encrypted data will be accessible only to Atra-Hasis users with knowledge of the appropriate password.

To turn off encryption and encode/decode plaintext, use the `-u` flag:
	
	./AtraHasis -u [input_image.png] [output_image.png] [text_file.txt]
	./AtraHasis -u [encoded_image.png]


Included Files
--------------

* `Makefile`
* `AtraHasis.c`, `AtraHasis.h` -- main program
* `Geshtu.c`, `Geshtu.h` -- for image encoding
* `Jarasandha.c`, `Jarasandha.h` -- for text encryption
* `gundam.png`, `test.png`, `test1.png`, `test2.png`, `test3.png` -- sample images
* `text_example.txt` -- short example text file
* `example2.txt` -- unicode text examples

Known Issues
------------

* Black line at the bottom of and unviewable thumbnail for encoded png files

Future Development
------------------

* Cleanup of known bugs

Programmers
-----------

� Jordan Kodner and Anand Sundaram, University of Pennsylvania School of Engineering and Applied Sciences, all rights reserved, 2012.