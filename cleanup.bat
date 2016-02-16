FOR /d /r . %%x IN (x64) DO @IF EXIST "%%x" rd /s /q "%%x"
cd bin
del *.ilk
del *.lastcodeanalysissucceeded
del *.obj
del *.idb
del *.pdb
del *.log
del *.tlog
del *.xml
FOR /d /r . %%x IN (*tlog) DO @IF EXIST "%%x" rd /s /q "%%x"
cd..