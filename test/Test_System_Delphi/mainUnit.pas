unit mainUnit;

interface

uses
  System.SysUtils, System.Types, System.UITypes, System.Classes, System.Variants,
  FMX.Types, FMX.Controls, FMX.Forms, FMX.Graphics, FMX.Dialogs, FMX.Edit,
  FMX.Layouts, FMX.Memo, FMX.Controls.Presentation, FMX.StdCtrls,
  Backend_Interface;

type
  TForm1 = class(TForm)
    RunScript: TButton;
    Editor: TMemo;
    LogW: TMemo;
    StopAq: TButton;
    procedure RunScriptClick(Sender: TObject);
    procedure StopAqClick(Sender: TObject);

  private
    { Private declarations }
  public
    procedure OverrideOnTimer(Sender: TObject);
    { Public declarations }
  end;

var
  Form1: TForm1;
  FTimer : TTimer;
  LogWp: ^TMemo;
implementation

{$R *.fmx}

procedure DBackend_RequestDataDelete(var a_dataname: cpp_string); cdecl;
begin
  Logwp.Lines.Add('>>> CALLBACK >>> DBackend_RequestDataDelete: ' + a_dataname);
end;

function DBackend_OnDataOut(var a_dataname: cpp_string; var a_fit_width: cpp_string; var a_display_type: cpp_string): Boolean; cdecl;
begin
  Logwp.Lines.Add('>>> CALLBACK >>> DBackend_OnDataOut: ' + a_dataname);
  Result := true;
end;

function DBackend_OnRefreshDataWindow(var a_dataname: cpp_string; var a_full_change: Boolean; var a_first_change: Boolean): PAnsiChar; cdecl;
var
  tmpp: string;
  TotalNumberOfSamples: array of LongInt;
  NrChannels: LongInt;
begin
  NrChannels := DGetNrChannels(a_dataname);
  SetLength(TotalNumberOfSamples, NrChannels);
  DGetTotalSamples(a_dataname, pLongInt(TotalNumberOfSamples));
  Logwp.Lines.Add('>>> CALLBACK >>> OnRefreshDataWindow: ' + a_dataname + ' Nr channels: ' + IntToStr(NrChannels) + ' Nr samples: ' + IntToStr(TotalNumberOfSamples[0]));
  Result := '';
end;

function DBackend_OnSystemExit(): PAnsiChar; cdecl;
begin
  Logwp.Lines.Add('>>> CALLBACK >>> DBackend_OnSystemExit');
end;

procedure DBackend_OnBindScrolling(var a_src_data_name: cpp_string; var a_dst_data_name: cpp_string; var a_remove_binding: cpp_string); cdecl;
begin
  Logwp.Lines.Add('>>> CALLBACK >>> DBackend_OnBindScrolling' + ' ' + a_src_data_name + ' ' + a_dst_data_name + ' ' + a_remove_binding);
end;

procedure TForm1.OverrideOnTimer(Sender: TObject);
begin
    DInterval();
end;

procedure TForm1.RunScriptClick(Sender: TObject);
var
  tmpstring: string;
  Script1, Script2, DataName: array[0..1999] of AnsiChar;
  NrChannels, i, j, NrSamplesToRead: LongInt;
  Starts, Stops, Enable: array of LongInt;
  Data: array of array of double;

  DataRaw1, DataRaw2: ppDouble;
  DataRaw3: pDouble;
  datarow4: aodpointer;

begin
  NrChannels := 1;
  NrSamplesToRead := 3;
  SetLength(Data, NrChannels);
  SetLength(Starts, NrChannels);
  SetLength(Stops, NrChannels);
  SetLength(Enable, NrChannels);

  for i := 0 to NrChannels - 1 do
  begin
    Starts[i] := 0;
    Stops[i] := NrSamplesToRead;
    Enable[i] := 1;
    SetLength(Data[i], NrSamplesToRead);
  end;

  DataRaw1 := @Data[0];
  datarow4 := @Data[0];


  dataName := 'vec';
  DataName := 'DATASERIES0';
  Script1 := 'CreateVector(DATASERIES0, 1 2 3); DisplayData(DATASERIES0); ';
  //Script := 'DataAq(7, D:/tamas/PSAA/Development/Test/Data/tasks_038/Bin/SeresJuditINA821_01.bdf, 2000 2000, 12 12, 0 1, 100, 4, FileName05.bdf);';
  Script2 := 'DataAq(6, , 2000 2000 2000 2000 2000 2000 2000 2000, 1 1 1 1 1 1 1 1, 0 1 2 3 4 5 6 7, 100, 4, orig_signal.bdf, orig_signal);';

  Logwp := @logw;

  DInitializeBackend(DBackend_RequestDataDelete, DBackend_OnDataOut, DBackend_OnRefreshDataWindow, DBackend_OnSystemExit, DBackend_OnBindScrolling);

  FTimer := TTimer.Create(nil);
  FTimer.OnTimer := OverrideOnTimer;
  FTimer.Interval := 20;
  FTimer.Enabled := true;

  Logw.Lines.Add('Script1 run result: ' + DRunScript(Script1, false).ToString);
  Logw.Lines.Add('Script2 run result: ' + DRunScript(Script2, false).ToString);

  Logw.Lines.Add('DGetDataRaw result: ' + DGetDataRaw(DataName, ppDouble(Data), pLongInt(Starts), pLongInt(Stops), pLongInt(Enable)).ToString);
  for i := 0 to NrChannels - 1 do
  begin
    Logw.Lines.Add('Data read (' + IntToStr(NrSamplesToRead) + ' samples, channel ' + IntToStr(i) + '): ');
    tmpstring := '[ ';
    for j := 0 to NrSamplesToRead - 1 do
    begin
      if j > 0 then
         tmpstring := tmpstring + ', ';
      //tmpstring := tmpstring + floatToStr(Data[i][j]);
      //tmpstring := tmpstring + floatToStr(pdouble(DataRaw3)^[j]);
      tmpstring := tmpstring + floatToStr(aodpointer(datarow4)^[j]);

    end;
    tmpstring := tmpstring + ' ]';
    Logw.Lines.Add(tmpstring);
  end;
end;

procedure TForm1.StopAqClick(Sender: TObject);
var
  Script1: array[0..1999] of AnsiChar;
  DataName1: array[0..299] of AnsiChar;
  DataName1Str: AnsiString;
  TotalNumberOfSamples: array of LongInt;
  NrChannels: LongInt;
  var AStr: AnsiString;
begin
  Logwp := @logw;
  DInitializeBackend(DBackend_RequestDataDelete, DBackend_OnDataOut, DBackend_OnRefreshDataWindow, DBackend_OnSystemExit, DBackend_OnBindScrolling);

  DataName1Str := 'orig_signal';
  Script1 := 'FileOpen(d:/tamas/PSAA/Development/Test/Data/tasks_038/Bin/SeresJuditINA821_01.bdf,' + ' orig_signal);';

//  Script1 := 'FileOpen(d:/tamas/PSAA/Development/Test/Data/tasks_038/Bin/SeresJuditINA821_01.bdf,' + (DataName1Str);
  AStr := AnsiString(DataName1Str);
  DataName1 := PByte(PAnsiChar(AStr));
  //  DataName1Str.ToCharArray(0, 5)
//  PByte(PAnsiChar(DataName1Str) + '' +

  DataName1 := 'orig_signal';
  DRunScript(Script1, false);

  NrChannels := DGetNrChannels(DataName1);
  SetLength(TotalNumberOfSamples, NrChannels);
  DGetTotalSamples(DataName1, pLongInt(TotalNumberOfSamples));
  Logwp.Lines.Add('Data opening: ' + DataName1 + ' Nr channels: ' + IntToStr(NrChannels) + ' Nr samples in first channel: ' + IntToStr(TotalNumberOfSamples[0]));

  DDestroyData('orig_signal');
end;

end.
