program CodecTest;

uses
  System.StartUpCopy,
  FMX.Forms,
  mainUnit in 'mainUnit.pas' {CodecTestForm},
  codecInterface in '..\..\codecInterface.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TCodecTestForm, CodecTestForm);
  Application.Run;
end.
