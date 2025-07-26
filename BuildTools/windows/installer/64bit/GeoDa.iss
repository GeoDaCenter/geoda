[Setup]
AppName=GeoDa                                                      
AppPublisher=GeoDa Center
AppPublisherURL=https://spatial.uchicago.edu/
AppSupportURL=https://spatial.uchicago.edu/
AppUpdatesURL=https://spatial.uchicago.edu/
AppSupportPhone=(480)965-7533
AppVersion=1.22.0.18
DefaultDirName={pf}\GeoDa Software
DefaultGroupName=GeoDa Software
; Since no icons will be created in "{group}", we don't need the wizard
; to ask for a Start Menu folder name:
;DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\GeoDa.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\..
OutputBaseFilename=GeoDa_1.22.0.18_x64_Setup
;OutputDir=userdocs:Inno Setup Examples Output

; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
; Note: We don't set ProcessorsAllowed because we want this
; installation to run on all architectures (including Itanium,
; since it's capable of running 32-bit code too).
ArchitecturesAllowed=x64

; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64

ChangesAssociations=yes

ShowLanguageDialog=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[dirs]
Name: "{app}";  Permissions: everyone-full; Check: InitializeSetup
Name: "{app}\basemap_cache";  Permissions: everyone-full
Name: "{app}\lang";  Permissions: everyone-full
Name: "{app}\proj";  Permissions: everyone-full

[Files]
Source: "..\..\Release\GeoDa.exe"; DestDir: "{app}"; DestName: "GeoDa.exe"; Check: IsX64
Source: "..\..\..\CommonDistFiles\GeoDa.ico"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\copyright.txt"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\GPLv3.txt"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\cache.sqlite"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\geoda_prefs.sqlite"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\geoda_prefs.json"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\web_plugins\*"; DestDir: "{app}\web_plugins"; Flags: recursesubdirs
Source: "..\..\..\CommonDistFiles\proj\*"; DestDir: "{app}\proj"; Flags: recursesubdirs

Source: "VC_redist.x64.exe"; DestDir: "{app}"
Source: "..\..\temp\OpenCL\sdk\bin\x64\OpenCL.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets\lib\vc_x64_dll\wxmsw314u_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets\lib\vc_x64_dll\wxmsw314u_gl_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\expat.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\freexl.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\gdal302.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\geos.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\geos_c.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\iconv.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libcrypto-1_1-x64.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libcurl.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libmysql.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libpq.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libssl-1_1-x64.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\libxml2.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\openjp2.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\proj.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\proj_6_1.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\spatialite.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\sqlite3.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\xerces-c_3_2.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\zlib1.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\gdal\plugins\ogr_OCI.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\gdal\plugins-optional\ogr_PG.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\gdal\plugins-optional\ogr_MSSQLSpatial.dll"; DestDir: "{app}"
Source: "..\..\libraries\bin\gdal\plugins-external\ogr_FileGDB.dll"; DestDir: "{app}"
Source: "..\..\..\..\Algorithms\lisa_kernel.cl"; DestDir: "{app}"
Source: "..\..\..\..\internationalization\lang\*"; DestDir: "{app}\lang"; Flags: recursesubdirs
; Add lang data back to {app} so they can be copied to other new windows users
Source: "..\..\..\..\internationalization\lang\*"; DestDir: "{app}\lang"; Flags: recursesubdirs
Source: "..\..\libraries\bin\gdal-data\*"; DestDir: "{app}\data"; Flags: recursesubdirs

;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\GeoDa"; Filename: "{app}\GeoDa.exe"
;Name: "{group}\GeoDa"; Filename: "{app}\run_geoda.bat"; IconFilename: "{app}\GeoDa.ico"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\GeoDa"; Filename: "{app}\GeoDa.exe"
;Name: "{commondesktop}\GeoDa"; Filename: "{app}\run_geoda.bat"; IconFilename: "{app}\GeoDa.ico"

[Registry]
; set PATH
; set GEODA_GDAL_DATA
; Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"GDAL_DATA"; ValueData:"{app}\data"; Flags: preservestringtype uninsdeletevalue
; set GEODA_OGR_DRIVER_PATH
; Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OGR_DRIVER_PATH"; ValueData:"{app}"; Flags: preservestringtype uninsdeletevalue
; Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"PROJ_LIB"; ValueData:"{app}\proj"; Flags: preservestringtype uninsdeletevalue

Root: HKCR; Subkey: ".gda"; ValueType: string; ValueName: ""; ValueData: "GeoDaProjectFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "GeoDaProjectFile"; ValueType: string; ValueName: ""; ValueData: "GeoDa Project File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "GeoDaProjectFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\GeoDa.exe,0"
Root: HKCR; Subkey: "GeoDaProjectFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\GeoDa.exe"" ""%1"""

; set Browser Emulation for wxWebView to IE 11.  IE 10 or earlier does not work with current D3
Root: "HKCU"; Subkey: "Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION"; ValueType: dword; ValueName:"GeoDa.exe"; ValueData:"$2AF9"

;Local Machine
Root: "HKLM"; Subkey: "SOFTWARE\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION"; ValueType: dword; ValueName: "GeoDa.exe"; ValueData: "$2AF9"

;64 Bit Mode
Root: "HKLM"; Subkey: "SOFTWARE\Wow6432Node\Microsoft\Internet Explorer\MAIN\FeatureControl\FEATURE_BROWSER_EMULATION"; ValueType: dword; ValueName: "GeoDa.exe"; ValueData: "$2AF9"; Check: IsWin64
;run as admin
;Root: "HKLM"; Subkey: "SOFTWARE\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers\"; ValueType: String; ValueName: "{app}\GeoDa.exe"; ValueData: "RUNASADMIN"; Flags: uninsdeletekeyifempty uninsdeletevalue; MinVersion: 0,6.1

[Code]
function IsX64: Boolean;
begin
  Result := Is64BitInstallMode and (ProcessorArchitecture = paX64);
end;

function VCRedistNeedsInstall: Boolean;
begin
  Result := not RegKeyExists(HKLM,'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\X64');
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

function GetUninstallString2: string;
var
  sUnInstPath: string;
  sUnInstallString: String;
begin
  Result := '';
  sUnInstPath := ExpandConstant('Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is2'); //Alternative App GUID/ID
  sUnInstallString := '';
  if not RegQueryStringValue(HKLM, sUnInstPath, 'UninstallString', sUnInstallString) then
    RegQueryStringValue(HKCU, sUnInstPath, 'UninstallString', sUnInstallString);
  Result := sUnInstallString;
end;

function IsUpgrade: Boolean;
begin
  Result := (GetUninstallString() <> '') or (GetUninstallString2() <> '');
end;

function IsGeoDaInstalled: Boolean;
begin
  Result := RegValueExists(HKEY_LOCAL_MACHINE,'Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is1', 'UninstallString') or
            RegValueExists(HKEY_LOCAL_MACHINE,'Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is2', 'UninstallString') or
            RegValueExists(HKEY_CURRENT_USER,'Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is1', 'UninstallString') or
            RegValueExists(HKEY_CURRENT_USER,'Software\Microsoft\Windows\CurrentVersion\Uninstall\GeoDa_is2', 'UninstallString');
end;

function UninstallExistingGeoDa: Boolean;
var
  iResultCode: Integer;
  sUnInstallString: string;
  UninstallSuccess: Boolean;
begin
  Result := True;
  UninstallSuccess := False;
  
  // Try first uninstall string
  sUnInstallString := GetUninstallString();
  if sUnInstallString <> '' then
  begin
    sUnInstallString := RemoveQuotes(sUnInstallString);
    if Exec(ExpandConstant(sUnInstallString), '/SILENT', '', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
    begin
      UninstallSuccess := True;
    end;
  end;
  
  // Try second uninstall string if first failed
  if not UninstallSuccess then
  begin
    sUnInstallString := GetUninstallString2();
    if sUnInstallString <> '' then
    begin
      sUnInstallString := RemoveQuotes(sUnInstallString);
      if Exec(ExpandConstant(sUnInstallString), '/SILENT', '', SW_HIDE, ewWaitUntilTerminated, iResultCode) then
      begin
        UninstallSuccess := True;
      end;
    end;
  end;
  
  // Wait a moment for uninstall to complete
  if UninstallSuccess then
  begin
    Sleep(2000);
  end;
end;

function InitializeSetup: Boolean;
var
  V: Integer;
  UninstallAttempted: Boolean;
begin
  Result := True; // in case when no previous version is found
  UninstallAttempted := False;
  
  if IsGeoDaInstalled then
  begin
    V := MsgBox('An existing version of GeoDa was detected. Would you like to automatically uninstall it before installing the new version?' + #13#10 + #13#10 + 
                'Click Yes to automatically uninstall the existing version.' + #13#10 +
                'Click No to cancel the installation.', mbInformation, MB_YESNO);
    if V = IDYES then
    begin
      UninstallAttempted := True;
      if UninstallExistingGeoDa() then
      begin
        // Check if uninstall was successful
        if IsGeoDaInstalled then
        begin
          V := MsgBox('The automatic uninstall may not have completed successfully. Would you like to continue with the installation anyway?' + #13#10 + #13#10 +
                      'Note: This may cause conflicts with the existing installation.', mbConfirmation, MB_YESNO);
          if V = IDNO then
            Result := False;
        end;
      end
      else
      begin
        V := MsgBox('Failed to automatically uninstall the existing version. Would you like to continue with the installation anyway?' + #13#10 + #13#10 +
                    'Note: This may cause conflicts with the existing installation.', mbConfirmation, MB_YESNO);
        if V = IDNO then
          Result := False;
      end;
    end
    else
      Result := False; //when older version present and user chose not to uninstall
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
    4:
    begin
      langCode := '153';  // Portuguese
    end;
    5:
    begin
      langCode := '79';  // French
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
  ComboBox.Items.Add('Portuguese');
  ComboBox.Items.Add('French');
  ComboBox.ItemIndex := 0;
  ComboBox.OnChange := @ComboBoxChange;
  langCode := '58';
end;

function getLangCode(Param: String): String;
begin
  Result :=  langCode;
end;

[INI]
Filename: "{app}\lang\config.ini"; Section: "Translation"; Key: "Language"; String: {code:getLangCode|{app}}

[Run]
Filename: {app}\VC_redist.x64.exe; StatusMsg: Installing Visual C++ Redistributable for Visual Studio 2019 (14.28.29913.0)...; Check: VCRedistNeedsInstall
