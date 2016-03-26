@echo off
SET /P ANSWER=Install all vulkan examples on attached device (Y/N)?
if /i {%ANSWER%}=={y} (goto :install) 
if /i {%ANSWER%}=={yes} (goto :install) 
goto :exit 

:install
call build-all.bat -deploy
goto finish

:exit
echo Cancelled

:finish