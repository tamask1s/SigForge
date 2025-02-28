unit Backend_Interface;

(*/**
 * \brief Delphi interface for SigForge's backend.
 */*)

interface
uses
{$IFDEF WIN32}
  Windows,
{$ELSE}
  Wintypes, WinProcs,
{$ENDIF}
  System.SysUtils, System.Types, System.UITypes, System.Classes, System.Variants,
  FMX.Types, FMX.Controls, FMX.Forms, FMX.Graphics, FMX.Dialogs, FMX.Edit,
  FMX.Layouts, FMX.Memo, FMX.Controls.Presentation, FMX.StdCtrls;

type
  ppDouble = ^pDouble;
  cpp_string = array[0..299] of AnsiChar;
  DBackend_RequestDataDelete_PROC_TYPE = procedure(var a_dataname: cpp_string);  cdecl;
  DBackend_OnDataOut_PROC_TYPE = function(var a_dataname: cpp_string; var a_fit_width: cpp_string; var a_display_type: cpp_string): Boolean; cdecl;
  DBackend_OnRefreshDataWindow_PROC_TYPE = function(var a_dataname: cpp_string; var a_full_change: Boolean; var a_first_change: Boolean): PAnsiChar; cdecl;
  DBackend_OnSystemExit_PROC_TYPE = function(): PAnsiChar; cdecl;
  DBackend_OnBindScrolling_PROC_TYPE = procedure(var a_src_data_name: cpp_string; var a_dst_data_name: cpp_string; var a_remove_binding: cpp_string); cdecl;

var
(*/**
 * \brief Initializes the library. It must be called before calling
 * \brief any other functions. 3 callback functions needs to be provided.
 * \param[in] OnDataOut Will be called when a data is created in the backend and can be diplayed.
 * \param[in] OnRefreshDataWindow Called when a data has been changed, and a window might be refreshed.
 * \param[in] OnSystemExit Called when the backend requesting for a system exit.
 * \return TODO: returns void
 */*)
  DInitializeBackend: function(RequestDataDelete: DBackend_RequestDataDelete_PROC_TYPE; OnDataOut: DBackend_OnDataOut_PROC_TYPE; OnRefreshDataWindow: DBackend_OnRefreshDataWindow_PROC_TYPE; OnSystemExit: DBackend_OnSystemExit_PROC_TYPE; OnBindScrolling: DBackend_OnBindScrolling_PROC_TYPE): LongInt cdecl {$IFDEF WIN32} stdcall {$ENDIF};
  //DInitializeBackend: function(OnDataOut: DBackend_OnDataOut_PROC_TYPE; OnRefreshDataWindow: DBackend_OnRefreshDataWindow_PROC_TYPE; OnSystemExit: DBackend_OnSystemExit_PROC_TYPE): LongInt cdecl {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetDataRaw - Reads the raw data }
{-* }
{-* aBuffer - address of the output buffer }
{-* aStart - start sample index vector }
{-* aNrElements - aNrElements for one (the first) channel. }
{-* aEnable - channel indexes conted from 0 to be included in the result. TODO: start and nr elements are unsigned while indexes are signed. Test, and correct it if needed. }
{-* }
{-* Fills the buffer with the raw data from start to stop indexes for each }
{-* channel. }
{= }
  DGetDataRaw: function(aDataName: PAnsiChar;
                        aBuffer: ppDouble;
                        aStart: PLongInt;
                        aNrElements: PLongInt;
                        aEnable: PLongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetNrChannels - Gets the number of channels }
{-* }
{-* aDataName - data name }
{-* }
{-* Returns the number of channels. }
{= }
  DGetNrChannels: function(aDataName: PAnsiChar):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetTotalSamples - Gets the total samples per channels }
{-* }
{-* aDataName - data name }
{-* aTotalSamples - vector of sample numbers }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetTotalSamples: function(aDataName: PAnsiChar;
                             aTotalSamples: PLongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetSampleRates - Gets the sample rates per channels }
{-* }
{-* aDataName - data name }
{-* aSampleRates - vector of sample rates }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetSampleRates: function(aDataName: PAnsiChar;
                            aSampleRates: PDouble):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetNrChannels - Gets the number of channels }
{-* }
{-* aScript - the script to run }
{-* }
{-* Returns the number of channels. }
{= }
  DRunScript: function(aScript: PAnsiChar; a_reload_plugins: Boolean):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DOpenFile - Opens a file }
{-* }
{-* aFileName - file name with full path }
{-* aDataName - optional. name of the data to be given by the caller }
{-* }
{-* Opens a file }
{-* Returns data name reference. If aDataName was given, then it should have the }
{-* same valu, if not, it has an automatically choosen name. }
{= }
  DOpenFile: function(aFileName: PAnsiChar;
                      aDataName: PAnsiChar):PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


(*/**
 * \brief DInterval.
 * \brief This function needs to be called in every 20ms in order for the backend to work properly.
 * \brief The backend is not thread safe, but it is not running any "visible" threads.
 * \brief All callback interactions of the backend towards its user will be called directly from this "Interval()" call chain.
 * \return TODO: return is void, pls correct it.
 */*)
  DInterval: function(): LongInt cdecl {$IFDEF WIN32} stdcall {$ENDIF};

  
{+//* DDestroyData - Deinitializes a data }
{-* }
{-* aDataName - name of the data }
{-* }
{-* Deletes the whole data codec structure. It also closes the file if it is open. }
{-* \return TODO: return is void, pls correct it.
{= }
  DDestroyData: function(aDataName: PAnsiChar):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetFunctionListRef  }
{-* }
{-* returns a list of script functions available in the signal processing libraries }
{-* TODO: describe format here }
{-* }
{= }
  DGetFunctionListRef: function():PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
 
 
{+//* DSetDescription }
{-* }
{-* aDataName - name of data }
{-* aDescription - sets description for the data }
{-* TODO: describe format here }
{-* TODO: void return }
{= }
  DSetDescription: function(aDataName: PAnsiChar;
                            aDescription: PAnsiChar):PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetDescription }
{-* }
{-* aDataName - name of data }
{-* return TODO: describe format here }
{= }
  DGetDescription: function(aDataName: PAnsiChar):PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetFileFormatExtensions }
{-* }
{-* return TODO: describe format here }
{= }
  DGetFileFormatExtensions: function():PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


{+//* DGetUnits }
{-* }
{-* aDataName - name of data }
{-* return TODO: describe format here }
{= }
  DGetUnits: function(aDataName: PAnsiChar):PAnsiChar cdecl  {$IFDEF WIN32} stdcall {$ENDIF};


  DBackend_DLLLoaded: Boolean
    {$IFDEF WIN32} = False; {$ENDIF}

procedure LoadBackendDLL;

implementation

var
  DBackend_SaveExit: pointer;
  DBackend_DLLHandle: THandle;
{$IFNDEF MSDOS}
  DBackend_ErrorMode: Integer;
{$ENDIF}

  procedure DBackend_NewExit; far;
  begin
    ExitProc := DBackend_SaveExit;
    FreeLibrary(DBackend_DLLHandle)
  end {DBackend_NewExit};

procedure LoadBackendDLL;
begin
  //ShowMessage('Start dll load proc');
  if DBackend_DLLLoaded then Exit;
{$IFNDEF MSDOS}
  DBackend_ErrorMode := SetErrorMode($8000{SEM_NoOpenFileErrorBox});
{$ENDIF}
  //ShowMessage('Before dll load');
  DBackend_DLLHandle := LoadLibrary('Backend.dll');
  //ShowMessage('After dll load');
  if DBackend_DLLHandle >= 32 then
  begin
    //ShowMessage('After dll load successful');
    DBackend_DLLLoaded := True;
    DBackend_SaveExit := ExitProc;
    ExitProc := @DBackend_NewExit;
    @DInitializeBackend := GetProcAddress(DBackend_DLLHandle,'InitializeBackend');
  {$IFDEF WIN32}
    Assert(@DInitializeBackend <> nil);
  {$ENDIF}
    @DGetDataRaw := GetProcAddress(DBackend_DLLHandle,'DGetDataRaw');
  {$IFDEF WIN32}
    Assert(@DGetDataRaw <> nil);
  {$ENDIF}
    @DGetNrChannels := GetProcAddress(DBackend_DLLHandle,'DGetNrChannels');
  {$IFDEF WIN32}
    Assert(@DGetNrChannels <> nil);
  {$ENDIF}
    @DGetTotalSamples := GetProcAddress(DBackend_DLLHandle,'DGetTotalSamples');
  {$IFDEF WIN32}
    Assert(@DGetTotalSamples <> nil);
  {$ENDIF}
    @DGetSampleRates := GetProcAddress(DBackend_DLLHandle,'DGetSampleRates');
  {$IFDEF WIN32}
    Assert(@DGetSampleRates <> nil);
  {$ENDIF}
    @DRunScript := GetProcAddress(DBackend_DLLHandle,'RunScript');
  {$IFDEF WIN32}
    Assert(@DRunScript <> nil);
  {$ENDIF}
    @DOpenFile := GetProcAddress(DBackend_DLLHandle,'DOpenFile');
  {$IFDEF WIN32}
    Assert(@DOpenFile <> nil);
  {$ENDIF}
    @DInterval := GetProcAddress(DBackend_DLLHandle,'Interval');
  {$IFDEF WIN32}
    Assert(@DInterval <> nil);
  {$ENDIF}
    @DDestroyData := GetProcAddress(DBackend_DLLHandle,'DestroyData');
  {$IFDEF WIN32}
    Assert(@DDestroyData <> nil);
  {$ENDIF}
    @DGetFunctionListRef := GetProcAddress(DBackend_DLLHandle,'GetFunctionListRef');
  {$IFDEF WIN32}
    Assert(@DGetFunctionListRef <> nil);
  {$ENDIF}
    @DSetDescription := GetProcAddress(DBackend_DLLHandle,'SetDescription');
  {$IFDEF WIN32}
    Assert(@DSetDescription <> nil);
  {$ENDIF}
    @DGetDescription := GetProcAddress(DBackend_DLLHandle,'GetDescription');
  {$IFDEF WIN32}
    Assert(@DGetDescription <> nil);
  {$ENDIF}
    @DGetFileFormatExtensions := GetProcAddress(DBackend_DLLHandle,'GetFileFormatExtensions');
  {$IFDEF WIN32}
    Assert(@DGetFileFormatExtensions <> nil);
  {$ENDIF}
    @DGetUnits := GetProcAddress(DBackend_DLLHandle,'GetUnits');
  {$IFDEF WIN32}
    Assert(@DGetUnits <> nil);
  {$ENDIF}
  end
  else
  begin
    DBackend_DLLLoaded := False;
    { Error: dll could not be loaded! }
  end;
{$IFNDEF MSDOS}
  SetErrorMode(DBackend_ErrorMode)
{$ENDIF}
end {LoadBackendDLL};

begin
 LoadBackendDLL;
end.
