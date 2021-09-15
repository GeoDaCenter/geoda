[Setup]
AppName=GeoDa                                                      
AppPublisher=GeoDa Center
AppPublisherURL=https://spatial.uchiago.edu/
AppSupportURL=https://spatial.uchiago.edu/
AppUpdatesURL=https://spatial.uchiago.edu/
AppSupportPhone=(480)965-7533
AppVersion=1.20
DefaultDirName={localappdata}\GeoDa
DefaultGroupName=GeoDa Software
; Since no icons will be created in "{group}", we don't need the wizard
; to ask for a Start Menu folder name:
;DisableProgramGroupPage=yes
UninstallDisplayIcon={localappdata}\GeoDa\GeoDa.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\..
OutputBaseFilename=GeoDa_1.20_win7+x86_Setup
;OutputDir=userdocs:Inno Setup Examples Output

ChangesAssociations=yes

ShowLanguageDialog=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"

[dirs]
Name: "{localappdata}\GeoDa";  Permissions: users-full; Check: InitializeSetup
Name: "{localappdata}\GeoDa\basemap_cache";  Permissions: users-full
Name: "{localappdata}\GeoDa\lang";  Permissions: users-full
Name: "{localappdata}\GeoDa\proj";  Permissions: users-full

[Files]
Source: "..\..\Release\GeoDa.exe"; DestDir: "{localappdata}\GeoDa"; DestName: "GeoDa.exe"
Source: "..\..\..\CommonDistFiles\GeoDa.ico"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\copyright.txt"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\GPLv3.txt"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\cache.sqlite"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\geoda_prefs.sqlite"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\geoda_prefs.json"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\CommonDistFiles\web_plugins\*"; DestDir: "{localappdata}\GeoDa\web_plugins"; Flags: recursesubdirs
Source: "..\..\..\CommonDistFiles\proj\*"; DestDir: "{localappdata}\GeoDa\proj"; Flags: recursesubdirs


Source: "VC_redist.x86.exe"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\temp\OpenCL\sdk\bin\x86\OpenCL.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\temp\wxWidgets\lib\vc_dll\wxmsw314u_vc_custom.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\temp\wxWidgets\lib\vc_dll\wxmsw314u_gl_vc_custom.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\expat.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\freexl.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\gdal302.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\geos.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\geos_c.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\iconv.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libcrypto-1_1.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libcurl.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libmysql.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libpq.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libssl-1_1.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\libxml2.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\openjp2.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\proj.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\proj_6_1.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\spatialite.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\sqlite3.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\xerces-c_3_2.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\zlib1.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\gdal\plugins\ogr_OCI.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\gdal\plugins-optional\ogr_PG.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\gdal\plugins-optional\ogr_MSSQLSpatial.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\libraries\bin\gdal\plugins-external\ogr_FileGDB.dll"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\..\Algorithms\lisa_kernel.cl"; DestDir: "{localappdata}\GeoDa"
Source: "..\..\..\..\internationalization\lang\*"; DestDir: "{localappdata}\GeoDa\lang"; Flags: recursesubdirs
Source: "..\..\libraries\bin\gdal-data\*"; DestDir: "{localappdata}\GeoDa\data"; Flags: recursesubdirs

;Source: "Readme.txt"; DestDir: "{localappdata}\GeoDa"; Flags: isreadme

[Icons]
Name: "{group}\GeoDa"; Filename: "{localappdata}\GeoDa\GeoDa.exe"
;Name: "{group}\GeoDa"; Filename: "{localappdata}\GeoDa\run_geoda.bat"; IconFilename: "{localappdata}\GeoDa\GeoDa.ico"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\GeoDa"; Filename: "{localappdata}\GeoDa\GeoDa.exe"
;Name: "{commondesktop}\GeoDa"; Filename: "{localappdata}\GeoDa\run_geoda.bat"; IconFilename: "{localappdata}\GeoDa\GeoDa.ico"

[Registry]
; set PATH
; set GEODA_GDAL_DATA
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"GDAL_DATA"; ValueData:"{localappdata}\GeoDa\data"; Flags: preservestringtype uninsdeletevalue
; set GEODA_OGR_DRIVER_PATH
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OGR_DRIVER_PATH"; ValueData:"{localappdata}\GeoDa"; Flags: preservestringtype uninsdeletevalue
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"PROJ_LIB"; ValueData:"{localappdata}\GeoDa\proj"; Flags: preservestringtype uninsdeletevalue

Root: HKCR; Subkey: ".gda"; ValueType: string; ValueName: ""; ValueData: "GeoDaProjectFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "GeoDaProjectFile"; ValueType: string; ValueName: ""; ValueData: "GeoDa Project File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "GeoDaProjectFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{localappdata}\GeoDa\GeoDa.exe,0"
Root: HKCR; Subkey: "GeoDaProjectFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{localappdata}\GeoDa\GeoDa.exe"" ""%1"""

; set Browser Emulation for wxWebView to IE 11.  IE 10 or earlier does not work with current D3
Root: "HKCU"; Subkey: "Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION"; ValueType: dword; ValueName:"GeoDa.exe"; ValueData:"$2AF9"

;Local Machine
Root: "HKLM"; Subkey: "SOFTWARE\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION"; ValueType: dword; ValueName: "GeoDa.exe"; ValueData: "$2AF9"

;run as admin
;Root: "HKLM"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers\"; ValueType: String; ValueName: "{localappdata}\GeoDa\GeoDa.exe"; ValueData: "RUNASADMIN"; Flags: uninsdeletekeyifempty uninsdeletevalue; MinVersion: 0,6.1

[Code]
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paX64);
end;

function IsIA64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paIA64);
end;

function IsOtherArch: Boolean;
begin
  Result := not IsX64 and not IsIA64;
end;

function VCRedistNeedsInstall: Boolean;
begin
  Result := not RegKeyExists(HKLM,'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{03d1453c-7d5c-479c-afea-8482f406e036}');
end;

function GetUninstallString: string;
var
  sUnInstPath: string;
  sUnInstallString: String;
begin
  Result := '';
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is1'); //Your App GUID/ID
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;

function IsUpgrade: Boolean;
begin
  Result := (GetUninstallString() <> '');
end;

function InitializeSetup: Boolean;
var
  V: Integer;
  iResultCode: Integer;
  sUnInstallString: string;
begin
  Result := True; // in case when no previous version is found
  if RegValueExists(HKEY_LOCAL_MACHINE,'Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is1', 'UninstallString') then  //Your App GUID/ID
  begin
    V := MsgBox(ExpandConstant('An old version of GeoDa was detected. Please uninstall it before continuing.'), mbInformation, MB_YESNO); //Custom Message if App installed
    if V = IDYES then
    begin
      sUnInstallString := GetUninstallString();
      sUnInstallString :=  RemoveQuotes(sUnInstallString);
      Exec(ExpandConstant(sUnInstallString), '', '', SW_SHOW, ewWaitUntilTerminated, iResultCode);
      Result := True; //if you want to proceed after uninstall
      //Exit; //if you want to quit after uninstall
    end
    else
      Result := False; //when older version present and not uninstalled
  end;
end;

var
  Button: TNewButton;
  ComboBox: TNewComboBox;
  CustomPage: TWizardPage;     
  langCode: string;

procedure ComboBoxChange(Sender: TObject);
begin
  case ComboBox.ItemIndex of
    0:
    begin
      langCode := '58';
    end;
    1:
    begin
      langCode := '45';   // chinese
    end;
    2:
    begin
      langCode := '179';  // spanish
    end;
    3:
    begin
      langCode := '159';  // russian
    end;
  end;
end;

procedure InitializeWizard;
var
  DescLabel: TLabel;
begin
  CustomPage := CreateCustomPage(wpSelectDir, 'Language Selection', 'Please select a language for GeoDa');

  DescLabel := TLabel.Create(WizardForm);
  DescLabel.Parent := CustomPage.Surface;
  DescLabel.Left := 0;
  DescLabel.Top := 0;
  DescLabel.Caption := '';

  ComboBox := TNewComboBox.Create(WizardForm);
  ComboBox.Parent := CustomPage.Surface;
  ComboBox.Left := 0;
  ComboBox.Top := DescLabel.Top + DescLabel.Height + 6;  
  ComboBox.Width := 220;
  ComboBox.Style := csDropDownList;
  ComboBox.Items.Add('English');
  ComboBox.Items.Add('Chinese (Simplified)');
  ComboBox.Items.Add('Spanish');
  ComboBox.Items.Add('Russian');
  ComboBox.ItemIndex := 0;
  ComboBox.OnChange := @ComboBoxChange;
  langCode := '58';
end;

function getLangCode(Param: String): String;
begin
  Result :=  langCode;
end;

[INI]
Filename: "{localappdata}\GeoDa\lang\config.ini"; Section: "Translation"; Key: "Language"; String: {code:getLangCode|{localappdata}\GeoDa}

[Run]
Filename: {localappdata}\GeoDa\VC_redist.x86.exe; StatusMsg: Installing Visual C++ Redistributable for Visual Studio 2019 (14.28.29913.0)...; Check: VCRedistNeedsInstall
