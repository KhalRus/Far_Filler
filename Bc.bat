cls
@echo off
Brc32 -R Filler.rc
bcc32 -c -a1 -R- -RT- -M- -v- Filler.cpp
tlink32 -s -m -M -Tpd -aa -v- Filler.obj, Filler.dll, , import32 cw32,, Filler.res
del *.map
del *.obj
del *.res