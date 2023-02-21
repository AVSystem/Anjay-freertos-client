@echo off
echo prebuild.bat : started > %1\\output.txt
set "asmfile=%1\\se_key.s"
::comment this line to force python
::python is used if windows executable not found
pushd %1\..\..\..\..\..\..\Middlewares\ST\STM32_Secure_Engine\Utilities\KeysAndImages
set basedir=%cd%
popd
goto exe:
goto py:
:exe
::line for window executable
echo Prebuild with windows executable
set "prepareimage=%basedir%\\win\\prepareimage\\prepareimage.exe"
set "python="
if exist %prepareimage% (
goto prebuild
)
:py
::line for python
echo Prebuild with python script
set "prepareimage=%basedir%\\prepareimage.py"
set "python=python "
echo "python: %prepareimage%" >> %1\\output.txt 2>>&1
:prebuild
set "crypto_h=%1\\..\\Inc\\se_crypto_config.h"

::clean
if exist %1\\crypto.txt (
  del %1\\crypto.txt
)
if exist %asmfile% (
  del %asmfile%
)
if exist %1\\postbuild.bat (
  del %1\\postbuild.bat
)

::get crypto name
set "command=%python%%prepareimage% conf %crypto_h% > %1\\crypto.txt"
%command%
IF %ERRORLEVEL% NEQ 0 goto error
set /P crypto=<%1\\crypto.txt >> %1\\output.txt 2>>&1
echo crypto %crypto% selected >> %1\\output.txt 2>>&1

set "cortex=V7M"

:: Tabulation before section is mandatory !
echo 	AREA ^|.SE_Key_Data^|, CODE>%asmfile%

if "%crypto%"=="SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM" (
  set "type=GCM"
  goto AES128
)
if "%crypto%"=="SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256" (
  set "type=CBC"
  goto AES128
)
if "%crypto%"=="SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256" (
  goto ECDSA
)
goto end

:AES128
set "oemkey=%1\\..\\Binary\\OEM_KEY_COMPANY1_key_AES_%type%.bin"
set "command=%python%%prepareimage% trans -k %oemkey% -f SE_ReadKey_1 -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

set "oemkey=%1\\..\\Binary\\OEM_KEY_COMPANY2_key_AES_%type%.bin"
IF NOT EXIST %oemkey% goto :AES128_end
set "command=%python%%prepareimage% trans -k %oemkey% -f SE_ReadKey_2 -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

set "oemkey=%1\\..\\Binary\\OEM_KEY_COMPANY3_key_AES_%type%.bin"
IF NOT EXIST %oemkey% goto :AES128_end
set "command=%python%%prepareimage% trans -k %oemkey% -f SE_ReadKey_3 -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

:AES128_end
if "%crypto%"=="SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM" goto end

:ECDSA
set "ecckey=%1\\..\\Binary\\ECCKEY1.txt"
set "command=%python%%prepareimage% trans -k %ecckey% -f SE_ReadKey_1_Pub -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

set "ecckey=%1\\..\\Binary\\ECCKEY2.txt"
IF NOT EXIST %ecckey% goto end
set "command=%python%%prepareimage% trans -k %ecckey% -f SE_ReadKey_2_Pub -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

set "ecckey=%1\\..\\Binary\\ECCKEY3.txt"
IF NOT EXIST %ecckey% goto end
set "command=%python%%prepareimage% trans -k %ecckey% -f SE_ReadKey_3_Pub -v %cortex% >> %asmfile%"
%command%
IF %ERRORLEVEL% NEQ 0 goto error

goto end


:end
echo     END >> %asmfile%
set "command=copy %1\\%crypto%.bat %1\\postbuild.bat"
%command%
IF %ERRORLEVEL% NEQ 0 goto error
exit 0
:error
echo %command% : failed >> %1\\output.txt 2>&1
echo %command% : failed
pause
exit 1