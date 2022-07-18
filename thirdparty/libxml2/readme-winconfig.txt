* before running the following commands you must be at a command
  prompt that has the visual studio win32 environment properly set up
  to build from the command line. this can usually be down by
  selecting the correct command prompt from the visual studio tools
  menu item. Or you can start a command prompt and run the correct
  batch file for the win32 enviroment usually found under the vc7
  visual studio install directory. 

* after setting up the visual studio win32 environment change to the
  libxml2/libxml2<version>/win32 directory 

* create debug libraries by running the following from the command
  line (note if this is the very frist time that configure.js is being
  run or would be the first time that nmake -f Makefile.msvc would be
  run, you can skip the clean step)

nmake -f Makefile.msvc clean

cscript configure.js iconv=no static=yes prefix=.\build\debug cruntime=/MTd debug=yes schematron=no incdir=.\include libdir=.\lib\debug\static sodir=.\lib\debug\sharedxb

nmake -f Makefile.msvc

nmake -f Makefile.msvc install

* create release libraries by running the following from the command
  line (note if this is the very frist time that configure.js is being
  run or would be the first time that nmake -f Makefile.msvc would be
  run, you can skip the clean step)

nmake -f Makefile.msvc clean

cscript configure.js iconv=no static=yes prefix=.\build\release cruntime=/MT debug=no schematron=no incdir=.\include libdir=.\lib\release\static sodir=.\lib\release\shared

nmake -f Makefile.msvc

nmake -f Makefile.msvc install


check in/commit:
* win32\lib (all its contents
* win32\include (all its contents)

note wind32\build does not need to be checked in


using libxml2 in a windows project:

include path: 

  thirdparty/libxml2/libxml2<version>/win32/include


libs:
* debug
  + thirdparty/libxml2/libxml2<version>/win32/lib/debug/static/libxml2_a.lib

* release
  + thirdparty/libxml2/libxml2<version>/win32/lib/release/static/libxml2_a.lib

where <version> in all the above is the version of libxml2 you want to use

e.g. to use version 2.7.3 version 

  libxml2-2.7.3

additional libs needed when linking:
* ws_32.lib
