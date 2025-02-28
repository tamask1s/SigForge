copy ..\..\..\src\Backend_DynamicLib\Backend.dll .\Debug
copy ..\..\..\SigForge\App\libfftw3-3.dll .\Debug
xcopy ..\..\..\SigForge\Codecs\*.dll .\Codecs /sy
xcopy ..\..\..\SigForge\InputModules\*.dll .\InputModules /sy
xcopy ..\..\..\SigForge\SignalProcessors\*.dll .\SignalProcessors /sy

	