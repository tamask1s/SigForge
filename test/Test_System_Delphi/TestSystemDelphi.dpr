program TestSystemDelphi;

uses
  System.StartUpCopy,
  FMX.Forms,
  mainUnit in 'mainUnit.pas' {Form1},
  Backend_Interface in '..\..\src\Backend\Interface\Delphi\Backend_Interface.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
