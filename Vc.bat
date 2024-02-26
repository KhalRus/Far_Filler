cls
@echo off
rc Filler.rc
cl /O1 /Zp2 /LD /Oi Filler.cpp /link /opt:nowin98 /noentry /def:Filler.def Filler.res kernel32.lib advapi32.lib
del *.obj
del *.res
del *.lib
del *.exp