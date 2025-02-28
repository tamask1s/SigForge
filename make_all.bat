pushd .
cd D:\tamas\PSAA\Development\Applications\SigForge\src\Frontend\Controls
g++.exe -O2 -I..\ -I..\Controls -I..\..\DataStructures -c D:\tamas\PSAA\Development\Applications\SigForge\src\Frontend\Controls\Bitmap.cpp -o .objs\Bitmap.o
g++.exe -O2 -I..\ -I..\Controls -I..\..\DataStructures -c D:\tamas\PSAA\Development\Applications\SigForge\src\Frontend\Controls\Control_Base.cpp -o .objs\Control_Base.o
g++.exe -O2 -I..\ -I..\Controls -I..\..\DataStructures -c D:\tamas\PSAA\Development\Applications\SigForge\src\Frontend\Controls\FTDevice.cpp -o .objs\FTDevice.o
cmd /c if exist libControlsStaticLib.a del libControlsStaticLib.a
ar.exe -r -s libControlsStaticLib.a .objs\Bitmap.o .objs\Control_Base.o .objs\FTDevice.o
popd
