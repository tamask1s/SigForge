unit codecInterface;

interface
uses
{$IFDEF WIN32}
  Windows;
{$ELSE}
  Wintypes, WinProcs;
{$ENDIF}
type
  ppDouble = ^pDouble;
var
{+//* DGetCodecInfo - Function to get information on the supported file type }
{-* }
{-* aExtensions - the list of supported file extensions, for example "*.txt;*.pdf" }
{-* aDescription - description of codec and the supported file types }
{-* aID - unique ID. This number identifies the codec. }
{-* }
{-* The ID should be globally unique within an application. }
{-* Two codecs shouldn't have the same ID. }
{= }
  DGetCodecInfo: function(aExtensions: PAnsiChar;
                          aDescription: PAnsiChar;
                          aID: PLongInt): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DInitData - Initializes a codec object. }
{-* }
{-* Returns codec handle. (It is the address of the data structure.) }
{-* Fills the data structure with the codec provided default values. }
{-* You must call this function in order to have a codec handle. }
{-* You should have a codec handle before using any other functionality. }
{-* }
{= }
  DInitData: function(): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DCloseFile - Close the previously opened file }
{-* }
{-* aHandle - codec handle }
{-* }
{-* Closes the previously opened file and performs the aditional memory }
{-* cleanups. }
{= }
  DCloseFile: function(aHandle: LongInt): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DDeleteData - Deinitializes the codec object }
{-* }
{-* aHandle - codec handle }
{-* }
{-* Deletes the whole data codec structure. It also closes the file if it is open. }
{-* The handle is freed up and is not usable any further. }
{= }
  DDeleteData: function(aHandle: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DOpenFile - Open a file }
{-* }
{-* aHandle - codec handle }
{-* aFileName - file name with full path }
{-* aWrite - true: Read/Write access, but no sharing }
{-* false: Read only access, but with sharing }
{-* }
{-* Opens a file, and fills up the data structure with the header of the file. }
{-* If Write is true then the file is opened with R/W access, but no sharing is }
{-* possible. If Write is false, the file is opened with RO access, read only }
{-* sharing is possible, but the DSaveHeader function is disabled. }
{= }
  DOpenFile: function(aHandle: LongInt; 
                      aFileName: PAnsiChar;
                      const aWrite: Bool):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DCreateNewFile - Create a file }
{-* }
{-* aHandle - codec handle }
{-* aFileName - file name with full path }
{-* aChannels = number of channels }
{-* aRewrite - true: rewrites existing file }
{-* false: check for existing file }
{-* }
{-* Creates a new file. If Rewrite is true then the file will be rewritten. }
{-* If Rewrite is false, the file will be checked, and function will fail }
{-* if the file exists. If the number of channels is above zero, then the }
{-* necessary memory alocations will be performed. }
{= }
  DCreateNewFile: function(aHandle: LongInt; 
                        aFileName: PAnsiChar; 
                        aChannels: LongInt; 
                        const aRewrite: Bool):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetDataRaw - Reads the raw data }
{-* }
{-* aHandle - codec handle }
{-* aBuffer - address of the output buffer }
{-* aStart - start sample index vector }
{-* aStop - stop sample index vector }
{-* }
{-* Fills the buffer with the raw data from start to stop indexes for each }
{-* channel. }
{= }
  DGetDataRaw: function(aHandle: LongInt; 
                        aBuffer: ppDouble;
                        aStart: PLongInt;
                        aStop: PLongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetChannelRaw - Reads the raw data by channels }
{-* }
{-* aData - address of the data structure }
{-* aBuffer - address of the output buffer }
{-* aEnable - channel enable index vector }
{-* aStart - start sample index vector }
{-* aStop - stop sample index vector }
{-* }
{-* Fills the buffer with the raw data from start to stop indexes for the }
{-* specified channels. Enable is an array with boolean values to specify }
{-* the active channels. }
{= }
  DGetChannelRaw: function(aHandle: LongInt;
                           aBuffer: ppDouble;
                           aEnable: PLongInt;
                           aStart: PLongInt;
                           aStop: PLongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetSingleChannelRaw - Reads the raw data of a single channel }
{-* }
{-* aData - address of the data structure }
{-* aBuffer - address of the output buffer }
{-* aEnable - index of the enabled channel }
{-* aStart - start sample index }
{-* aStop - stop sample index }
{-* }
{-* Fills the buffer with the raw data from start to stop indexes for the }
{-* specified channel. Enable is the index of the channel. }
{= }
  DGetSingleChannelRaw: function(aHandle: LongInt;
                           aBuffer: pDouble;
                           aEnable: LongInt;
                           aStart: LongInt;
                           aStop: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DWriteHeader - Writes file header }
{-* }
{-* aData - address of the data structure }
{= }
  DWriteHeader: function(aHandle: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DWriteBlock - writes raw data }
{-* }
{-* aHandle - codec handle }
{-* aBuffer - address of the buffer to write }
{-* aStart - start sample index vector }
{-* aStop - stop sample index vector }
{-* }
{-* Writes the buffer with the raw data to the file }
{-* from start to stop indexes for each channel. }
{-* TotalSamples are not incremented. }
{= }
  DWriteBlock: function(aHandle: LongInt;
                        Buffer: ppDouble;
                        Start: LongInt;
                        Stop: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DAppendSamples - appends raw data }
{-* }
{-* aHandle - codec handle }
{-* aBuffer - address of the buffer to write }
{-* NrSamples - number of samples to write }
{-* }
{-* Appends the buffer with the raw data to the file }
{-* TotalSamples are incremented with NrSamples. }
{= }
  DAppendSamples: function(aHandle: LongInt;
                           Buffer: ppDouble;
                           NrSamples: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetNrChannels - Gets the number of channels }
{-* }
{-* aHandle - codec handle }
{-* }
{-* Returns the number of channels. }
{= }
  DGetNrChannels: function(aHandle: LongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetTotalSamples - Gets the total samples per channels }
{-* }
{-* aHandle - codec handle }
{-* aTotalSamples - vector of sample numbers }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetTotalSamples: function(aHandle: LongInt;
                             aTotalSamples: PLongInt):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetSampleRates - Gets the sample rates per channels }
{-* }
{-* aHandle - codec handle }
{-* aSampleRates - vector of sample rates }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetSampleRates: function(aHandle: LongInt;
                            aSampleRates: PDouble):LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetLabel - Gets the label of one channel }
{-* }
{-* aHandle - codec handle }
{-* aChannelIndex - index of channel }
{-* aLabel - label string }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetLabel: function(aHandle: LongInt;
                      aChannelIndex: LongInt;
                      aLabel: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetVerticalUnit - Gets the vertical unit of one channel }
{-* }
{-* aHandle - codec handle }
{-* aChannelIndex - index of channel }
{-* aVerticalUnit - vertical unit }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetVerticalUnit: function(aHandle: LongInt;
                             aChannelIndex: LongInt;
                             aVerticalUnit: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetHorizontalUnit - Gets the horiyontal unit }
{-* }
{-* aHandle - codec handle }
{-* aHorizontalUnit - horiyontal unit }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetHorizontalUnit: function(aHandle: LongInt;
                               aHorizontalUnit: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetRecording - Gets the recording description }
{-* }
{-* aHandle - codec handle }
{-* aRecording - recording description }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetRecording: function(aHandle: LongInt;
                          aRecording: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetDate - Gets the date of recording }
{-* }
{-* aHandle - codec handle }
{-* aDate - date of recording }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetDate: function(aHandle: LongInt;
                     aDate: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetTime - Gets the time of recording }
{-* }
{-* aHandle - codec handle }
{-* aTime - time of recording }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetTime: function(aHandle: LongInt;
                     aTime: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DGetFileName - Gets the current file name }
{-* }
{-* aHandle - codec handle }
{-* aFileName - file name }
{-* }
{-* Fills the aTotalSamples with the sample numbers for each channel. }
{= }
  DGetFileName: function(aHandle: LongInt;
                         aFileName: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

{+//* DSetSampleRates }
{-* }
{-* aHandle - codec handle }
{-* aSampleRates - array of sample rates }
{-* }
{-* Sets sample rates for each channel. }
{= }
  DSetSampleRates: function(aHandle: LongInt;
                            aSampleRates: PDouble): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							
{+//* DSetPhysicalMin }
{-* }
{-* aHandle - codec handle }
{-* aPhysicalMin - array of phisical min values }
{-* }
{-* Sets phisical min values for each channel. }
{= }
  DSetPhysicalMin: function(aHandle: LongInt;
                            aPhysicalMin: PDouble): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							
{+//* DSetPhysicalMax }
{-* }
{-* aHandle - codec handle }
{-* aPhysicalMin - array of phisical max values }
{-* }
{-* Sets phisical max values for each channel. }
{= }
  DSetPhysicalMax: function(aHandle: LongInt;
                            aPhysicalMax: PDouble): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							
{+//* DSetRecordSamples }
{-* }
{-* aHandle - codec handle }
{-* aRecordSamples - number record samples for each channel }
{-* }
{-* Many file types stores their data in blocks or records of samples, storing for example 1 sec of data for each channel. }
{-* Each "aRecordSamples" value is the number of samples in one record for one channel. }
{= }							
  DSetRecordSamples: function(aHandle: LongInt;
                              aRecordSamples: PLongInt): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							  
{+//* DSetRecordDuration }
{-* }
{-* aHandle - codec handle }
{-* aRecordDuration - duration of a record }
{-* }
{-* Many file types stores their data in blocks or records of samples, storing for example 1 sec of data for each channel. }
{-* This function sets the lenght of the data record. }
{= }
  DSetRecordDuration: function(aHandle: LongInt;
                               aRecordDuration: Double): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							   
{+//* DSetTotalSamples }
{-* }
{-* aHandle - codec handle }
{-* aTotalSamples - array of sample numbers }
{-* }
{-* Sets the number of total samples for each channel. }
{= }
  DSetTotalSamples: function(aHandle: LongInt;
                             aTotalSamples: PLongInt): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							 
{+//* DSetLabel }
{-* }
{-* aHandle - codec handle }
{-* aChannelIndex - index of channel }
{-* aLabel - label }
{-* }
{-* Sets the label for each channel. }
{= }
  DSetLabel: function(aHandle: LongInt;
                      aChannelIndex: LongInt;
                      aLabel: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
					  
{+//* DSetVerticalUnit }
{-* }
{-* aHandle - codec handle }
{-* aChannelIndex - index of channel }
{-* aVerticalUnit - vertical unit }
{-* }
{-* Sets the vertical unit for each channel. }
{= }
  DSetVerticalUnit: function(aHandle: LongInt;
                             aChannelIndex: LongInt;
                             aVerticalUnit: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							 
{+//* DSetHorizontalUnit }
{-* }
{-* aHandle - codec handle }
{-* aHorizontalUnit - horizontal unit }
{-* }
{-* Sets the horizontal unit of the file. }
{= }
  DSetHorizontalUnit: function(aHandle: LongInt;
                               aHorizontalUnit: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
							   
{+//* DSetRecording }
{-* }
{-* aHandle - codec handle }
{-* aRecording - recording information }
{-* }
{-* Sets the recording information of the file. }
{= }
  DSetRecording: function(aHandle: LongInt;
                          aRecording: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
						  
{+//* DSetDate }
{-* }
{-* aHandle - codec handle }
{-* aDate - date }
{-* }
{-* Sets the date of the file. }
{= }
  DSetDate: function(aHandle: LongInt;
                     aDate: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};
					 
{+//* DSetTime }
{-* }
{-* aHandle - codec handle }
{-* aTime - time }
{-* }
{-* Sets the time of the file. }
{= }
  DSetTime: function(aHandle: LongInt;
                     aTime: PAnsiChar): LongInt cdecl  {$IFDEF WIN32} stdcall {$ENDIF};

  DLLLoaded: Boolean
    {$IFDEF WIN32} = False; {$ENDIF}

implementation

var
  SaveExit: pointer;
  DLLHandle: THandle;
{$IFNDEF MSDOS}
  ErrorMode: Integer;
{$ENDIF}

  procedure NewExit; far;
  begin
    ExitProc := SaveExit;
    FreeLibrary(DLLHandle)
  end {NewExit};

procedure LoadDLL;
begin
  if DLLLoaded then Exit;
{$IFNDEF MSDOS}
  ErrorMode := SetErrorMode($8000{SEM_NoOpenFileErrorBox});
{$ENDIF}
  DLLHandle := LoadLibrary('..\..\..\..\..\..\SigForge\Codecs\Codec_BDF.dll');
  if DLLHandle >= 32 then
  begin
    DLLLoaded := True;
    SaveExit := ExitProc;
    ExitProc := @NewExit;
    @DGetCodecInfo := GetProcAddress(DLLHandle,'DGetCodecInfo');
  {$IFDEF WIN32}
    Assert(@DGetCodecInfo <> nil);
  {$ENDIF}
    @DInitData := GetProcAddress(DLLHandle,'DInitData');
  {$IFDEF WIN32}
    Assert(@DInitData <> nil);
  {$ENDIF}
    @DCloseFile := GetProcAddress(DLLHandle,'DCloseFile');
  {$IFDEF WIN32}
    Assert(@DCloseFile <> nil);
  {$ENDIF}
    @DDeleteData := GetProcAddress(DLLHandle,'DDeleteData');
  {$IFDEF WIN32}
    Assert(@DDeleteData <> nil);
  {$ENDIF}
    @DOpenFile := GetProcAddress(DLLHandle,'DOpenFile');
  {$IFDEF WIN32}
    Assert(@DOpenFile <> nil);
  {$ENDIF}
    @DCreateNewFile := GetProcAddress(DLLHandle,'DCreateNewFile');
  {$IFDEF WIN32}
    Assert(@DCreateNewFile <> nil);
  {$ENDIF}
    @DGetDataRaw := GetProcAddress(DLLHandle,'DGetDataRaw');
  {$IFDEF WIN32}
    Assert(@DGetDataRaw <> nil);
  {$ENDIF}
    @DGetChannelRaw := GetProcAddress(DLLHandle,'DGetChannelRaw');
  {$IFDEF WIN32}
    Assert(@DGetChannelRaw <> nil);
  {$ENDIF}
    @DGetSingleChannelRaw := GetProcAddress(DLLHandle,'DGetSingleChannelRaw');
  {$IFDEF WIN32}
    Assert(@DGetSingleChannelRaw <> nil);
  {$ENDIF}
    @DWriteHeader := GetProcAddress(DLLHandle,'DWriteHeader');
  {$IFDEF WIN32}
    Assert(@DWriteHeader <> nil);
  {$ENDIF}
    @DWriteBlock := GetProcAddress(DLLHandle,'DWriteBlock');
  {$IFDEF WIN32}
    Assert(@DWriteBlock <> nil);
  {$ENDIF}
    @DAppendSamples := GetProcAddress(DLLHandle,'DAppendSamples');
  {$IFDEF WIN32}
    Assert(@DAppendSamples <> nil);
  {$ENDIF}
    @DGetNrChannels := GetProcAddress(DLLHandle,'DGetNrChannels');
  {$IFDEF WIN32}
    Assert(@DGetNrChannels <> nil);
  {$ENDIF}
    @DGetTotalSamples := GetProcAddress(DLLHandle,'DGetTotalSamples');
  {$IFDEF WIN32}
    Assert(@DGetTotalSamples <> nil);
  {$ENDIF}
    @DGetSampleRates := GetProcAddress(DLLHandle,'DGetSampleRates');
  {$IFDEF WIN32}
    Assert(@DGetSampleRates <> nil);
  {$ENDIF}
    @DGetLabel := GetProcAddress(DLLHandle,'DGetLabel');
  {$IFDEF WIN32}
    Assert(@DGetLabel <> nil);
  {$ENDIF}
    @DGetVerticalUnit := GetProcAddress(DLLHandle,'DGetVerticalUnit');
  {$IFDEF WIN32}
    Assert(@DGetVerticalUnit <> nil);
  {$ENDIF}
    @DGetHorizontalUnit := GetProcAddress(DLLHandle,'DGetHorizontalUnit');
  {$IFDEF WIN32}
    Assert(@DGetHorizontalUnit <> nil);
  {$ENDIF}
    @DGetRecording := GetProcAddress(DLLHandle,'DGetRecording');
  {$IFDEF WIN32}
    Assert(@DGetRecording <> nil);
  {$ENDIF}
    @DGetDate := GetProcAddress(DLLHandle,'DGetDate');
  {$IFDEF WIN32}
    Assert(@DGetDate <> nil);
  {$ENDIF}
    @DGetTime := GetProcAddress(DLLHandle,'DGetTime');
  {$IFDEF WIN32}
    Assert(@DGetTime <> nil);
  {$ENDIF}
    @DGetFileName := GetProcAddress(DLLHandle,'DGetFileName');
  {$IFDEF WIN32}
    Assert(@DGetFileName <> nil);
  {$ENDIF}
    @DSetSampleRates := GetProcAddress(DLLHandle,'DSetSampleRates');
  {$IFDEF WIN32}
    Assert(@DSetSampleRates <> nil);
  {$ENDIF}
    @DSetPhysicalMin := GetProcAddress(DLLHandle,'DSetPhysicalMin');
  {$IFDEF WIN32}
    Assert(@DSetPhysicalMin <> nil);
  {$ENDIF}
    @DSetPhysicalMax := GetProcAddress(DLLHandle,'DSetPhysicalMax');
  {$IFDEF WIN32}
    Assert(@DSetPhysicalMax <> nil);
  {$ENDIF}
    @DSetRecordSamples := GetProcAddress(DLLHandle,'DSetRecordSamples');
  {$IFDEF WIN32}
    Assert(@DSetRecordSamples <> nil);
  {$ENDIF}
    @DSetRecordDuration := GetProcAddress(DLLHandle,'DSetRecordDuration');
  {$IFDEF WIN32}
    Assert(@DSetRecordDuration <> nil);
  {$ENDIF}
    @DSetTotalSamples := GetProcAddress(DLLHandle,'DSetTotalSamples');
  {$IFDEF WIN32}
    Assert(@DSetTotalSamples <> nil);
  {$ENDIF}
    @DSetLabel := GetProcAddress(DLLHandle,'DSetLabel');
  {$IFDEF WIN32}
    Assert(@DSetLabel <> nil);
  {$ENDIF}
    @DSetVerticalUnit := GetProcAddress(DLLHandle,'DSetVerticalUnit');
  {$IFDEF WIN32}
    Assert(@DSetVerticalUnit <> nil);
  {$ENDIF}
    @DSetHorizontalUnit := GetProcAddress(DLLHandle,'DSetHorizontalUnit');
  {$IFDEF WIN32}
    Assert(@DSetHorizontalUnit <> nil);
  {$ENDIF}
    @DSetRecording := GetProcAddress(DLLHandle,'DSetRecording');
  {$IFDEF WIN32}
    Assert(@DSetRecording <> nil);
  {$ENDIF}
    @DSetDate := GetProcAddress(DLLHandle,'DSetDate');
  {$IFDEF WIN32}
    Assert(@DSetDate <> nil);
  {$ENDIF}
    @DSetTime := GetProcAddress(DLLHandle,'DSetTime');
  {$IFDEF WIN32}
    Assert(@DSetTime <> nil);
  {$ENDIF}
  end
  else
  begin
    DLLLoaded := False;
    { Error: Codec_BDF.dll could not be loaded ! }
  end;
{$IFNDEF MSDOS}
  SetErrorMode(ErrorMode)
{$ENDIF}
end {LoadDLL};

begin
  LoadDLL;
end.
