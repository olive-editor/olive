!include "MUI2.nsh"

!define MUI_ICON "install icon.ico"
!define MUI_UNICON "uninstall icon.ico"

!define APP_NAME "Oak Video Editor"
!define APP_TARGET "oak-editor"

!define MUI_FINISHPAGE_RUN "$INSTDIR\oak-editor.exe"

SetCompressor lzma

Name "${APP_NAME}"
OutFile "setup.exe"

ManifestDPIAware true
Unicode true

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
!define MUI_FINISHPAGE_RUN_FUNCTION "LaunchOak"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

Section "Oak Video Editor"
    SectionIn RO
    SetOutPath $INSTDIR
    File /r oak-editor\*

    # Render worker process must live next to the editor binary
    File "oak-editor\oak-render-worker.exe"

    # Render backends must also live next to the editor binary
    File "oak-editor\oakgl.dll"
    File /nonfatal "oak-editor\oakvulkan.dll"

    WriteUninstaller "$INSTDIR\uninstall.exe"

    # Install Visual C++ 2010 Redistributable
    #File "vcredist_x64.exe"
    #ExecWait '"$INSTDIR\vcredist_x64.exe" /quiet'
    #Delete "$INSTDIR\vcredist_x64.exe"
SectionEnd

Section "Create Desktop shortcut"
    CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_TARGET}.exe"
SectionEnd

Section "Create Start Menu shortcut"
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_TARGET}.exe"
    CreateShortCut "$SMPROGRAMS\${APP_NAME}\Uninstall ${APP_NAME}.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Associate *.ove files with Oak Video Editor"
    WriteRegStr HKCR ".ove" "" "OakEditor.OVEFile"
    WriteRegStr HKCR ".ove" "Content Type" "application/vnd.olive-project"
    WriteRegStr HKCR "OakEditor.OVEFile" "" "Oak project file"
    WriteRegStr HKCR "OakEditor.OVEFile\DefaultIcon" "" "$INSTDIR\oak-editor.exe,1"
    WriteRegStr HKCR "OakEditor.OVEFile\shell\open\command" "" "$\"$INSTDIR\oak-editor.exe$\" $\"%1$\""
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

UninstPage uninstConfirm
UninstPage instfiles

Section "uninstall"

    rmdir /r "$INSTDIR"

    Delete "$DESKTOP\${APP_NAME}.lnk"
    rmdir /r "$SMPROGRAMS\${APP_NAME}"

    DeleteRegKey HKCR ".ove"
    DeleteRegKey HKCR "OakEditor.OVEFile"
    DeleteRegKey HKCR "OakEditor.OVEFile\DefaultIcon" ""
    DeleteRegKey HKCR "OakEditor.OVEFile\shell\open\command" ""
    System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v (0x08000000, 0, 0, 0)'
SectionEnd

Function LaunchOak
    ExecShell "" "$INSTDIR\${APP_TARGET}.exe"
FunctionEnd
