#!/bin/bash - 
echo "SBSFU_UPDATE started"
# Absolute path to this script
SCRIPT=$(readlink -f $0)
# Absolute path this script
SCRIPTPATH=`dirname $SCRIPT`
PATH="~/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/":$PATH
PATH="/c/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin":$PATH
stm32programmercli="STM32_Programmer_CLI"
connect_no_reset="-c port=SWD mode=UR"
connect="-c port=SWD mode=UR --hardRst"
slot0=0xc016000
slot1=0x0
slot2=0xc0bc000
slot3=0x0
boot=0xc002000
loader=0x0
cfg_loader=0
image_number=1
if [ $image_number == 2 ]; then
echo "Write SBSFU_Appli Secure"
$stm32programmercli $connect -d $SCRIPTPATH/../../UserApp/BG96/Binary/sbsfu_s_init.bin $slot0 -v
ret=$?
if [ $ret != 0 ]; then
  if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script failed, press a key" -n1 -s; fi
  exit 1
fi
echo "SBSFU_Appli Secure Written"
echo "Write SBSFU_Appli NonSecure"
$stm32programmercli $connect -d $SCRIPTPATH/../../UserApp/BG96/Binary/sbsfu_ns_init.bin $slot1 -v 
ret=$?
if [ $ret != 0 ]; then
  if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script failed, press a key" -n1 -s; fi
  exit 1
fi
echo "SBSFU_Appli NonSecure Written"
fi
if [ $image_number == 1 ]; then
echo "Write SBSFU_Appli"
$stm32programmercli $connect -d $SCRIPTPATH/../../UserApp/BG96/Binary/sbsfu_init.bin $slot0 -v
ret=$?
if [ $ret != 0 ]; then
  if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script failed, press a key" -n1 -s; fi
  exit 1
fi
echo "SBSFU_Appli Written"
fi
# write loader if config loader is active
if [ $cfg_loader == 1 ]; then
echo "Write SBSFU_Loader"
$stm32programmercli $connect -d $SCRIPTPATH/../../SBSFU_Loader/Binary/loader.bin $loader -v
ret=$?
if [ $ret != 0 ]; then
  if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script failed, press a key" -n1 -s; fi
  exit 1
fi
echo "SBSFU_Loader Written"
fi
echo "Write SBSFU_Boot"
$stm32programmercli $connect -d $SCRIPTPATH/Release/SBSFU_Boot.bin $boot -v
ret=$?
if [ $ret != 0 ]; then
  if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script failed, press a key" -n1 -s; fi
  exit 1
fi
echo "SBSFU_Boot Written"
if [ "$1" != "AUTO" ]; then read -p "SBSFU_UPDATE script Done, press a key" -n1 -s; fi
exit 0
