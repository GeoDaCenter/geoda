[Setup]
AppName=GeoDa                                                      
AppPublisher=GeoDa Center
AppPublisherURL=http://geoda.asu.edu/
AppSupportURL=http://geoda.asu.edu/
AppUpdatesURL=http://geoda.asu.edu/
AppSupportPhone=(480)965-7533
AppVersion=1.5
DefaultDirName={pf}\GeoDa Software
DefaultGroupName=GeoDa Software
; Since no icons will be created in "{group}", we don't need the wizard
; to ask for a Start Menu folder name:
;DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\GeoDa.exe
Compression=lzma2
SolidCompression=yes
OutputDir=..\..
OutputBaseFilename=geoda_setup
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

[dirs]

[Files]
Source: "..\..\Release\GeoDa.exe"; DestDir: "{app}"; DestName: "GeoDa.exe"; Check: IsX64
Source: "..\..\..\CommonDistFiles\copyright.txt"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\GPLv3.txt"; DestDir: "{app}"

Source: "vcredist_x64.exe"; DestDir: "{app}"
Source: "ogr_FileGDB.dll"; DestDir: "{app}"
Source: "ogr_OCI.dll"; DestDir: "{app}"
Source: "ogr_SDE.dll"; DestDir: "{app}"
Source: "..\..\cache.sqlite"; DestDir: "{app}"
Source: "..\..\temp\expat-2.1.0\build\Release\expat.dll"; DestDir: "{app}"
Source: "..\..\temp\gdal-1.9.2\gdal19.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\lib\libpq.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\ssleay32.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\libintl-8.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\libeay32.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets-3.0.0\lib\vc_x64_dll\wxmsw30u_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets-3.0.0\lib\vc_x64_dll\wxmsw30u_gl_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_54_0\stage\lib\boost_chrono-vc100-mt-1_54.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_54_0\stage\lib\boost_thread-vc100-mt-1_54.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_54_0\stage\lib\boost_system-vc100-mt-1_54.dll"; DestDir: "{app}"

Source: "..\..\..\..\SampleData\Examples\*"; DestDir: "{userdocs}\Examples"; Flags: recursesubdirs uninsneveruninstall

Source: "..\..\temp\gdal-1.9.2\data\*"; DestDir: "{app}\data"; Flags: recursesubdirs

;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\GeoDa"; Filename: "{app}\GeoDa.exe"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\GeoDa"; Filename: "{app}\GeoDa.exe"

[Registry]
; set PATH
; set GDAL_DATA
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OGR_DATA"; ValueData:"{pf}\GeoDa Software\data"; Flags: preservestringtype
; set OGR_DRIVER_PATH
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"OGR_DRIVER_PATH"; ValueData:"{pf}\GeoDa Software"; Flags: preservestringtype

Root: HKCR; Subkey: ".gda"; ValueType: string; ValueName: ""; ValueData: "GeoDaProjectFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "GeoDaProjectFile"; ValueType: string; ValueName: ""; ValueData: "GeoDa Project File"; Flags: uninsdeletekey
Root: HKCR; Subkey: "GeoDaProjectFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\GeoDa.exe,0"
Root: HKCR; Subkey: "GeoDaProjectFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\GeoDa.exe"" ""%1"""

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
  Result := not RegKeyExists(HKLM,'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{1D8E6291-B0D5-35EC-8441-6616F567A0F7}');
end;

[Run]
Filename: {app}\vcredist_x64.exe; StatusMsg: Installing Visual Studio 2010 SP1 C++ CRT Libraries...; Check: VCRedistNeedsInstall
