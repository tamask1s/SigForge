unit mainUnit;

interface

uses
  System.SysUtils, System.Types, System.UITypes, System.Classes, System.Variants,
  FMX.Types, FMX.Controls, FMX.Forms, FMX.Graphics, FMX.Dialogs,
  FMX.Controls.Presentation, FMX.Memo, FMX.StdCtrls, codecInterface,
  FMX.Layouts;

type
  TCodecTestForm = class(TForm)
    Memo1: TMemo;
    BtnReadFile: TButton;
    BtnCreateFile: TButton;
    procedure BtnReadFileClick(Sender: TObject);
    procedure BtnCreateFileClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  CodecTestForm: TCodecTestForm;


implementation

{$R *.fmx}

procedure TCodecTestForm.BtnReadFileClick(Sender: TObject);
var
  FileExtensions, FileTypeDescription, FileName : array[0..199] of AnsiChar;
  CodecHandle, ID, NrChannels, OpenResult, NrSamplesToRead, i, j : LongInt;
  Data: array of array of double;
  tmpstring : String;
  DataArray, SampleRates : array of Double;
  TotalSamples, Starts, Stops : array of LongInt;
begin
  { Getting information on the type of the file encoder }
  DGetCodecInfo(FileExtensions, FileTypeDescription, @ID);  { <------- API call ------- }
  Memo1.Lines.Add('FileExtensions: ' + FileExtensions);
  Memo1.Lines.Add('FileTypeDescription: ' + FileTypeDescription);
  Memo1.Lines.Add('ID: ' + IntToStr(ID));
  { Creating a file encoder handle }
  CodecHandle := DInitData();
  FileName := 'SimulatedSine.bdf';
  Memo1.Lines.Add('File name: ' + FileName);
  { Opening a data file }
  OpenResult := DOpenFile(CodecHandle, FileName, FALSE);  { <------- API call ------- }
  Memo1.Lines.Add('File open result: ' + IntToStr(OpenResult));
  if OpenResult < 1 then
    exit;
  { Getting numer of channels }
  NrChannels := DGetNrChannels(CodecHandle);  { <------- API call ------- }
  Memo1.Lines.Add('NrChannels: ' + IntToStr(NrChannels));

  SetLength(TotalSamples, NrChannels);
  { Getting numer of total sample in each channel }
  DGetTotalSamples(CodecHandle, pLongInt(TotalSamples));  { <------- API call ------- }
  tmpstring := 'TotalSamples: [ ';
  for i := 0 to NrChannels - 1 do
  begin
    if i > 0 then
      tmpstring := tmpstring + ', ';
    tmpstring := tmpstring + IntToStr(TotalSamples[i]);
  end;
  tmpstring := tmpstring + ' ]';
  Memo1.Lines.Add(tmpstring);

  SetLength(SampleRates, NrChannels);
  { Getting numer sample rates of each channel }
  DGetSampleRates(CodecHandle, pDouble(SampleRates));  { <------- API call ------- }
  tmpstring := 'SampleRates: [ ';
  for i := 0 to NrChannels - 1 do
  begin
    if i > 0 then
      tmpstring := tmpstring + ', ';
    tmpstring := tmpstring + FloatToStr(SampleRates[i]);
  end;
  tmpstring := tmpstring + ' ]';
  Memo1.Lines.Add(tmpstring);

  { Allocating structures for readnig multichannel raw data }
  NrSamplesToRead := 6;
  SetLength(Starts, NrSamplesToRead);
  SetLength(Stops, NrSamplesToRead);
  SetLength(Data, NrChannels);
  for i := 0 to NrChannels - 1 do
  begin
    Starts[i] := 0;
    Stops[i] := NrSamplesToRead;
    SetLength(Data[i], NrSamplesToRead);
  end;

  { Readnig multichannel raw data }
  DGetDataRaw(CodecHandle, ppDouble(Data), pLongInt(Starts), pLongInt(Stops));  { <------- API call ------- }
  for i := 0 to NrChannels - 1 do
  begin
    Memo1.Lines.Add('Data read (' + IntToStr(NrSamplesToRead) + ' samples, channel ' + IntToStr(i) + ': ');
    tmpstring := '[ ';
    for j := 0 to NrSamplesToRead - 1 do
    begin
      if j > 0 then
         tmpstring := tmpstring + ', ';
      tmpstring := tmpstring + floatToStr(Data[i][j]);
    end;
    tmpstring := tmpstring + ' ]';
    Memo1.Lines.Add(tmpstring);
  end;

  { Allocating structures for readnig single channel raw data }
  SetLength(DataArray, NrSamplesToRead);
  { Readnig single channel raw data }
  DGetSingleChannelRaw(CodecHandle, pDouble(DataArray), 0, 0, NrSamplesToRead);  { <------- API call ------- }
  Memo1.Lines.Add('Single channel read (' + IntToStr(NrSamplesToRead) + ' samples): ');
  tmpstring := '[ ';
  for j := 0 to NrSamplesToRead - 1 do
  begin
    if j > 0 then
       tmpstring := tmpstring + ', ';
    tmpstring := tmpstring + FloatToStr(DataArray[j]);
  end;
  tmpstring := tmpstring + ' ]';
  Memo1.Lines.Add(tmpstring);
  { Closing file encoder }
  DDeleteData(CodecHandle);  { <------- API call ------- }
end;

procedure TCodecTestForm.BtnCreateFileClick(Sender: TObject);
var
  CodecHandle, NrChannels, RecIndx, NrRecords, SamplingRate, i, j, NrSamplesPerChannel: LongInt;
  FileName : array[0..199] of AnsiChar;
  RecordLength : Double;
  IntBuf : array of LongInt;
  DoubleBuf : array of Double;
  Data: array of array of double;
begin
  CodecHandle := DInitData();
  FileName  := 'SimulatedSine.bdf';
  NrChannels := 3;
  SamplingRate := 1024;
  RecordLength := 1; { 2 seconds }
  DCreateNewFile(CodecHandle, FileName, NrChannels, true);  { <------- API call ------- }
  DSetRecordDuration(CodecHandle, RecordLength);  { <------- API call ------- }
  SetLength(IntBuf, NrChannels);
  SetLength(DoubleBuf, NrChannels);

  NrSamplesPerChannel := Round(RecordLength * SamplingRate);
  for i := 0 to NrChannels - 1 do
    IntBuf[i] := NrSamplesPerChannel;
  DSetRecordSamples(CodecHandle, pLongInt(IntBuf));  { <------- API call ------- }
  for i := 0 to NrChannels - 1 do
    DoubleBuf[i] := SamplingRate;
  DSetSampleRates(CodecHandle, pDouble(DoubleBuf));  { <------- API call ------- }
  for i := 0 to NrChannels - 1 do
    DoubleBuf[i] := -100;
  DSetPhysicalMin(CodecHandle, pDouble(DoubleBuf));  { <------- API call ------- }
  for i := 0 to NrChannels - 1 do
    DoubleBuf[i] := 100;
  DSetPhysicalMax(CodecHandle, pDouble(DoubleBuf));  { <------- API call ------- }
  DWriteHeader(CodecHandle);

  SetLength(Data, NrChannels);
  for i := 0 to NrChannels - 1 do
  begin
    SetLength(Data[i], NrSamplesPerChannel);
  end;

  NrRecords := 39; { Writing 39 * 2 second * 1024 samples / second = 79872 samples for each channel}
  for RecIndx := 0 to NrRecords - 1 do
  begin
      for j := 0 to NrChannels - 1 do
      begin
          for i := 0 to NrSamplesPerChannel - 1 do
              Data[j][i] := sin(i / 1024.0 * 2.0 * 3.14 * (RecIndx + 1)) * 100.0;
      end;
      DAppendSamples(CodecHandle, ppDouble(Data), NrSamplesPerChannel);  { <------- API call ------- }
  end;

  DWriteHeader(CodecHandle);  { <------- API call ------- }
  DDeleteData(CodecHandle);  { <------- API call ------- }
  Memo1.Lines.Add('File: ' + FileName + ' created.');
end;

end.
