!include "MUI.nsh"

!define MUI_ICON "install icon.ico"
!define MUI_UNICON "uninstall icon.ico"

!define APP_NAME "Olive"
!define APP_TARGET "olive-editor"

!define MUI_FINISHPAGE_RUN "$INSTDIR\olive-editor.exe"

SetCompressor lzma

Name ${APP_NAME}


!ifdef X64
InstallDir "$PROGRAMFILES64\${APP_NAME}"
!else
InstallDir "$PROGRAMFILES32\${APP_NAME}"
!endif

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE LICENSE
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN_TEXT "Run ${APP_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchOlive"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Olive (required)"
	
	SectionIn RO

	SetOutPath $INSTDIR

	File /r olive\*

	WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

Section "Create Desktop shortcut"
	CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_TARGET}.exe"
SectionEnd

Section "Create Start Menu shortcut"
	CreateDirectory "$SMPROGRAMS\${APP_NAME}"
	CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_TARGET}.exe"
	CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Associate *.ove files with Olive"
	WriteRegStr HKCR ".ove" "" "OliveEditor.OVEFile"
	WriteRegStr HKCR ".ove" "Content Type" "application/vnd.olive-project"
	WriteRegStr HKCR "OliveEditor.OVEFile" "" "Olive project file"
	WriteRegStr HKCR "OliveEditor.OVEFile\DefaultIcon" "" "$INSTDIR\olive-editor.exe,1"
	WriteRegStr HKCR "OliveEditor.OVEFile\shell\open\command" "" "$\"$INSTDIR\olive-editor.exe$\" $\"%1$\""
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

UninstPage uninstConfirm
UninstPage instfiles

Section "uninstall"

	rmdir /r "$INSTDIR"

	Delete "$DESKTOP\${APP_NAME}.lnk"
	rmdir /r "$SMPROGRAMS\${APP_NAME}"
	
	DeleteRegKey HKCR ".ove"
	DeleteRegKey HKCR "OliveEditor.OVEFile"
	DeleteRegKey HKCR "OliveEditor.OVEFile\DefaultIcon" ""
	DeleteRegKey HKCR "OliveEditor.OVEFile\shell\open\command" ""
	System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

Function LaunchOlive
	ExecShell "" "$INSTDIR\${APP_TARGET}.exe"
FunctionEnd