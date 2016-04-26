[Setup]
AppName=GeoDa                                                      
AppPublisher=GeoDa Center
AppPublisherURL=https://geodacenter.asu.edu/
AppSupportURL=https://geodacenter.asu.edu/
AppUpdatesURL=https://geodacenter.asu.edu/
AppSupportPhone=(480)965-7533
AppVersion=1.8
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
Name: "{app}\basemap_cache";  Permissions: everyone-full

[Files]
Source: "..\..\Release\GeoDa.exe"; DestDir: "{app}"; DestName: "GeoDa.exe"; Check: IsX64
Source: "..\..\..\CommonDistFiles\copyright.txt"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\GPLv3.txt"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\cache.sqlite"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\geoda_prefs.sqlite"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\geoda_prefs.json"; DestDir: "{app}"
Source: "..\..\..\CommonDistFiles\web_plugins\*"; DestDir: "{app}\web_plugins"; Flags: recursesubdirs

Source: "vcredist_x64.exe"; DestDir: "{app}"
Source: "ogr_FileGDB.dll"; DestDir: "{app}"
Source: "ogr_OCI.dll"; DestDir: "{app}"
Source: "ogr_SDE.dll"; DestDir: "{app}"
Source: "..\..\Release\sqlite.dll"; DestDir: "{app}"
Source: "..\..\temp\curl-7.46.0\builds\curlib\bin\libcurl.dll"; DestDir: "{app}"
Source: "..\..\temp\expat-2.1.0\build\Release\expat.dll"; DestDir: "{app}"
Source: "..\..\temp\libspatialite-4.0.0\spatialite.dll"; DestDir: "{app}"
Source: "..\..\temp\geos-3.3.8\src\geos_c.dll"; DestDir: "{app}"
Source: "..\..\temp\freexl-1.0.0e\freexl.dll"; DestDir: "{app}"
Source: "..\..\temp\proj-4.8.0\src\proj.dll"; DestDir: "{app}"
Source: "..\..\temp\gdal\gdal_geoda20.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\lib\libpq.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\ssleay32.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\libintl-8.dll"; DestDir: "{app}"
Source: "..\..\temp\pgsql\bin\libeay32.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets-3.0.2\lib\vc_x64_dll\wxmsw30u_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\temp\wxWidgets-3.0.2\lib\vc_x64_dll\wxmsw30u_gl_vc_custom.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_57_0\stage\lib\boost_chrono-vc100-mt-1_57.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_57_0\stage\lib\boost_thread-vc100-mt-1_57.dll"; DestDir: "{app}"
Source: "..\..\temp\boost_1_57_0\stage\lib\boost_system-vc100-mt-1_57.dll"; DestDir: "{app}"

Source: "..\..\..\..\SampleData\Examples\*"; DestDir: "{userdocs}\Examples"; Flags: recursesubdirs uninsneveruninstall


Source: "..\..\temp\gdal\data\*"; DestDir: "{app}\data"; Flags: recursesubdirs

;Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme

[Icons]
Name: "{group}\GeoDa"; Filename: "{app}\GeoDa.exe"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"
Name: "{commondesktop}\GeoDa"; Filename: "{app}\GeoDa.exe"

[Registry]
; set PATH
; set GEODA_GDAL_DATA
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"GEODA_GDAL_DATA"; ValueData:"{pf}\GeoDa Software\data"; Flags: preservestringtype uninsdeletevalue
; set GEODA_OGR_DRIVER_PATH
Root: HKCU; Subkey: "Environment"; ValueType:string; ValueName:"GEODA_OGR_DRIVER_PATH"; ValueData:"{pf}\GeoDa Software"; Flags: preservestringtype uninsdeletevalue

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
