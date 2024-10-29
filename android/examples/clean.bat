FOR /d /r . %%d IN (assets) DO @IF EXIST "%%d" rd /s /q "%%d"
FOR /d /r . %%d IN (build) DO @IF EXIST "%%d" rd /s /q "%%d"