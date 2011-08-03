This is vcam "capture source filter" http://tmhare.mvps.org/downloads.htm
updated for VS 2010

To use it, you'll need to install VS 2010 express, the Microsoft SDK thus:
http://betterlogic.com/roger/?p=3096

Then examine the project properties and tweak the paths to match your own system.

In sum, what was necessary was creating a new VS 2010 "dll" project, import the old files, 
adding #include "stdafx.h" to each, and adding the paths as stated in the above blog.
