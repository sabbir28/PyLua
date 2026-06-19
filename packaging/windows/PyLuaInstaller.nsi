# PyLua Windows Installer (NSIS)
# This script is parameterized by the release workflow. Pass values with /D:
#   /DPRODUCT_VERSION=YYYY.MM.DD.N
#   /DSOURCE_DIR=<staging directory containing pylua.exe and runtime files>
#   /DOUTPUT_FILE=<absolute path to PyLua-<version>-Setup.exe>

Unicode true
RequestExecutionLevel admin
SetCompressor /SOLID lzma

!ifndef PRODUCT_VERSION
  !define PRODUCT_VERSION "0.0.0.0"
!endif

!ifndef SOURCE_DIR
  !define SOURCE_DIR "dist\\windows"
!endif

!ifndef OUTPUT_FILE
  !define OUTPUT_FILE "PyLua-${PRODUCT_VERSION}-Setup.exe"
!endif

!define PRODUCT_NAME "PyLua"
!define PRODUCT_PUBLISHER "PyLua Project"
!define PRODUCT_EXE "pylua.exe"
!define INSTALL_REG_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\${PRODUCT_NAME}"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${OUTPUT_FILE}"
InstallDir "$PROGRAMFILES64\\${PRODUCT_NAME}"
InstallDirRegKey HKLM "${INSTALL_REG_KEY}" "InstallLocation"

VIProductVersion "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "CompanyName" "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} Installer"
VIAddVersionKey /LANG=1033 "FileVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${PRODUCT_VERSION}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright (c) ${PRODUCT_PUBLISHER}"

!include "MUI2.nsh"
!include "x64.nsh"

!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\\Contrib\\Graphics\\Icons\\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\\Contrib\\Graphics\\Icons\\modern-uninstall.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install ${PRODUCT_NAME}" SEC_INSTALL
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "This installer requires 64-bit Windows."
    Abort
  ${EndIf}

  SetOutPath "$INSTDIR"
  File /r "${SOURCE_DIR}\\*"

  WriteUninstaller "$INSTDIR\\Uninstall.exe"

  CreateDirectory "$SMPROGRAMS\\${PRODUCT_NAME}"
  CreateShortcut "$SMPROGRAMS\\${PRODUCT_NAME}\\${PRODUCT_NAME}.lnk" "$INSTDIR\\${PRODUCT_EXE}"
  CreateShortcut "$SMPROGRAMS\\${PRODUCT_NAME}\\Uninstall ${PRODUCT_NAME}.lnk" "$INSTDIR\\Uninstall.exe"

  WriteRegStr HKLM "${INSTALL_REG_KEY}" "DisplayName" "${PRODUCT_NAME} ${PRODUCT_VERSION}"
  WriteRegStr HKLM "${INSTALL_REG_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr HKLM "${INSTALL_REG_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegStr HKLM "${INSTALL_REG_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${INSTALL_REG_KEY}" "DisplayIcon" "$INSTDIR\\${PRODUCT_EXE}"
  WriteRegStr HKLM "${INSTALL_REG_KEY}" "UninstallString" '"$INSTDIR\\Uninstall.exe"'
  WriteRegDWORD HKLM "${INSTALL_REG_KEY}" "NoModify" 1
  WriteRegDWORD HKLM "${INSTALL_REG_KEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\\${PRODUCT_NAME}\\${PRODUCT_NAME}.lnk"
  Delete "$SMPROGRAMS\\${PRODUCT_NAME}\\Uninstall ${PRODUCT_NAME}.lnk"
  RMDir "$SMPROGRAMS\\${PRODUCT_NAME}"

  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "${INSTALL_REG_KEY}"
SectionEnd
