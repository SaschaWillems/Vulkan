FOR /d /r . %%x IN (assets) DO @IF EXIST "%%x" rd /s /q "%%x"
FOR /d /r . %%x IN (bin) DO @IF EXIST "%%x" rd /s /q "%%x"
FOR /d /r . %%x IN (libs) DO @IF EXIST "%%x" rd /s /q "%%x"
FOR /d /r . %%x IN (obj) DO @IF EXIST "%%x" rd /s /q "%%x"
FOR /d /r . %%x IN (res) DO @IF EXIST "%%x" rd /s /q "%%x"
FOR /d /r . %%x IN (jni) DO @IF EXIST "%%x" rd /s /q "%%x"
del /s build.xml
del /s local.properties
del /s proguard-project.txt
del /s project.properties