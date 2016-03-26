rem @echo off
if NOT EXIST %1 (
	echo Please specify a valid project folder for running the build
) else (
	mkdir bin
	cd %1
	IF NOT EXIST build.xml (
		xcopy "..\_setup.bat" ".\" /Y
		call _setup.bat
		del _setup.bat
	)
	call build %2
	IF EXIST vulkan%1.apk (
		if "%2" == "-deploy" (
            echo deploying to device
			IF EXIST vulkan%1.apk (
			    adb install -r vulkan%1.apk
			)
		)	
		xcopy vulkan%1.apk "..\bin\" /Y
		del vulkan%1.apk /q
	)
	cd..
)