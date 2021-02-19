**WinFile**

Anything you can do with a MS-Windows system file.

It's a collection of functions that you can perform on a MS-Windows file.
Some of these are considderably faster than the POSIX or ifstream counterparts.

- More then twice the speed of bare FILE* access
- Four times the speed of \<iostream\> access

Works on a 128K internal buffer with the same size as the technical
driver buffer of SSD and HDD drives and file systems.

*See the Main.cpp file for a bunch of test functions*

*See the WinFile.h file for the complete interface*

*See the Winfile.docx document for a description*