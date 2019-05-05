make -f Makefile.OS2 build_release
emxbind -s releasei386\qwcl.exe
emxbind -s releasei386\qwcl_x11.exe
emxbind -s releasei386\qwsv.exe
copy releasei386\*.exe ..\..
