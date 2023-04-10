# This file contains options for producing a minimal build of wxWidgets and is
# included when compiling the library from source. All of the components that
# we aren't using should be switched off to improve the build time, however,
# please note that some of them are required for normal functioning of
# wxWidgets' internals (case in point: wxUSE_DYNLIB_CLASS). The full list of
# compilation options can be found here:
# <https://github.com/wxWidgets/wxWidgets/blob/v3.2.2.1/build/cmake/options.cmake>.
# If you need something that isn't enabled - just comment the set() line out.

set(wxUSE_REGEX OFF)
# set(wxUSE_ZLIB OFF)
set(wxUSE_EXPAT OFF)
set(wxUSE_LIBJPEG OFF)
# set(wxUSE_LIBPNG OFF)
set(wxUSE_LIBTIFF OFF)
set(wxUSE_NANOSVG OFF)

set(wxUSE_LIBLZMA OFF)

set(wxUSE_OPENGL OFF)

if(UNIX)
  set(wxUSE_LIBSDL OFF)
  # set(wxUSE_LIBICONV OFF)
  set(wxUSE_NOTIFY OFF)
  set(wxUSE_XTEST OFF)
  set(wxUSE_LIBMSPACK OFF)
  set(wxUSE_GTKPRINT OFF)
  set(wxUSE_LIBGNOMEVFS OFF)
  set(wxUSE_GLCANVAS_EGL OFF)
endif()

# set(wxUSE_INTL OFF)
# set(wxUSE_XLOCALE OFF)
# set(wxUSE_CONFIG OFF)

set(wxUSE_SOCKETS OFF)
set(wxUSE_IPV6 OFF)
if(WIN32)
  # set(wxUSE_OLE OFF)
endif()
# set(wxUSE_DATAOBJ OFF)

set(wxUSE_IPC OFF)

# set(wxUSE_ANY OFF)
# set(wxUSE_APPLE_IEEE OFF)
set(wxUSE_ARCHIVE_STREAMS OFF)
set(wxUSE_BASE64 OFF)
# set(wxUSE_STACKWALKER OFF)
# set(wxUSE_ON_FATAL_EXCEPTION OFF)
# set(wxUSE_CMDLINE_PARSER OFF)
# set(wxUSE_DATETIME OFF)
set(wxUSE_DEBUGREPORT OFF)
set(wxUSE_DIALUP_MANAGER OFF)
# set(wxUSE_DYNLIB_CLASS OFF)
# set(wxUSE_DYNAMIC_LOADER OFF)
# set(wxUSE_EXCEPTIONS OFF)
# set(wxUSE_EXTENDED_RTTI OFF)
# set(wxUSE_FFILE OFF)
# set(wxUSE_FILE OFF)
set(wxUSE_FILE_HISTORY OFF)
set(wxUSE_FILESYSTEM OFF)
set(wxUSE_FONTENUM OFF)
# set(wxUSE_FONTMAP OFF)
set(wxUSE_FS_ARCHIVE OFF)
set(wxUSE_FS_INET OFF)
set(wxUSE_FS_ZIP OFF)
set(wxUSE_FSVOLUME OFF)
set(wxUSE_FSWATCHER OFF)
# set(wxUSE_GEOMETRY OFF)
# set(wxUSE_LOG OFF)
# set(wxUSE_LONGLONG OFF)
set(wxUSE_MIMETYPE OFF)
# set(wxUSE_PRINTF_POS_PARAMS OFF)
set(wxUSE_SECRETSTORE OFF)
set(wxUSE_SNGLINST_CHECKER OFF)
set(wxUSE_SOUND OFF)
set(wxUSE_SPELLCHECK OFF)
# set(wxUSE_STDPATHS OFF)
# set(wxUSE_STOPWATCH OFF)
# set(wxUSE_STREAMS OFF)
# set(wxUSE_SYSTEM_OPTIONS OFF)
set(wxUSE_TARSTREAM OFF)
# set(wxUSE_TEXTBUFFER OFF)
# set(wxUSE_TEXTFILE OFF)
# set(wxUSE_TIMER OFF)
# set(wxUSE_VARIANT OFF)

set(wxUSE_WEBREQUEST OFF)

set(wxUSE_ZIPSTREAM OFF)

set(wxUSE_URL OFF)
set(wxUSE_PROTOCOL OFF)
set(wxUSE_PROTOCOL_HTTP OFF)
set(wxUSE_PROTOCOL_FTP OFF)
set(wxUSE_PROTOCOL_FILE OFF)

# set(wxUSE_THREADS OFF)

if(WIN32)
  # set(wxUSE_DBGHELP OFF)
  # set(wxUSE_INICONF OFF)
  # set(wxUSE_WINSOCK2 OFF)
  # set(wxUSE_REGKEY OFF)
endif()

set(wxUSE_DOC_VIEW_ARCHITECTURE OFF)
set(wxUSE_HELP OFF)
set(wxUSE_MS_HTML_HELP OFF)
set(wxUSE_HTML OFF)
set(wxUSE_WXHTML_HELP OFF)
set(wxUSE_XRC OFF)
set(wxUSE_XML OFF)
set(wxUSE_AUI OFF)
set(wxUSE_PROPGRID OFF)
set(wxUSE_RIBBON OFF)
set(wxUSE_STC OFF)
set(wxUSE_CONSTRAINTS OFF)
# set(wxUSE_LOGGUI OFF)
set(wxUSE_LOGWINDOW OFF)
set(wxUSE_LOG_DIALOG OFF)
set(wxUSE_MDI OFF)
set(wxUSE_MDI_ARCHITECTURE OFF)
set(wxUSE_MEDIACTRL OFF)
set(wxUSE_RICHTEXT OFF)
set(wxUSE_POSTSCRIPT OFF)
set(wxUSE_AFM_FOR_POSTSCRIPT OFF)
set(wxUSE_PRINTING_ARCHITECTURE OFF)
set(wxUSE_SVG OFF)
set(wxUSE_WEBVIEW OFF)

# set(wxUSE_GRAPHICS_CONTEXT OFF)
# set(wxUSE_CAIRO OFF)

set(wxUSE_CLIPBOARD OFF)
set(wxUSE_DRAG_AND_DROP OFF)

# set(wxUSE_MARKUP OFF)

# set(wxUSE_ACCEL OFF)
set(wxUSE_ACTIVITYINDICATOR OFF)
set(wxUSE_ADDREMOVECTRL OFF)
set(wxUSE_ANIMATIONCTRL OFF)
set(wxUSE_BANNERWINDOW OFF)
# set(wxUSE_ARTPROVIDER_STD OFF)
# set(wxUSE_ARTPROVIDER_TANGO OFF)
# set(wxUSE_BMPBUTTON OFF)
set(wxUSE_BITMAPCOMBOBOX OFF)
# set(wxUSE_BUTTON OFF)
set(wxUSE_CALENDARCTRL OFF)
set(wxUSE_CARET OFF)
# set(wxUSE_CHECKBOX OFF)
set(wxUSE_CHECKLISTBOX OFF)
# set(wxUSE_CHOICE OFF)
set(wxUSE_CHOICEBOOK OFF)
set(wxUSE_COLLPANE OFF)
set(wxUSE_COLOURPICKERCTRL OFF)
# set(wxUSE_COMBOBOX OFF)
set(wxUSE_COMBOCTRL OFF)
set(wxUSE_COMMANDLINKBUTTON OFF)
set(wxUSE_DATAVIEWCTRL OFF)
set(wxUSE_NATIVE_DATAVIEWCTRL OFF)
set(wxUSE_DATEPICKCTRL OFF)
set(wxUSE_DETECT_SM OFF)
set(wxUSE_DIRPICKERCTRL OFF)
# set(wxUSE_DISPLAY OFF)
set(wxUSE_EDITABLELISTBOX OFF)
set(wxUSE_FILECTRL OFF)
set(wxUSE_FILEPICKERCTRL OFF)
set(wxUSE_FONTPICKERCTRL OFF)
# set(wxUSE_GAUGE OFF)
# set(wxUSE_GRID OFF)
# set(wxUSE_HEADERCTRL OFF)
set(wxUSE_HYPERLINKCTRL OFF)
# set(wxUSE_IMAGLIST OFF)
set(wxUSE_INFOBAR OFF)
set(wxUSE_LISTBOOK OFF)
# set(wxUSE_LISTBOX OFF)
# set(wxUSE_LISTCTRL OFF)
set(wxUSE_NOTEBOOK OFF)
set(wxUSE_NOTIFICATION_MESSAGE OFF)
set(wxUSE_ODCOMBOBOX OFF)
set(wxUSE_POPUPWIN OFF)
set(wxUSE_PREFERENCES_EDITOR OFF)
set(wxUSE_RADIOBOX OFF)
# set(wxUSE_RADIOBTN OFF)
set(wxUSE_RICHMSGDLG OFF)
set(wxUSE_RICHTOOLTIP OFF)
set(wxUSE_REARRANGECTRL OFF)
set(wxUSE_SASH OFF)
# set(wxUSE_SCROLLBAR OFF)
set(wxUSE_SEARCHCTRL OFF)
set(wxUSE_SLIDER OFF)
# set(wxUSE_SPINBTN OFF)
# set(wxUSE_SPINCTRL OFF)
set(wxUSE_SPLITTER OFF)
# set(wxUSE_STATBMP OFF)
# set(wxUSE_STATBOX OFF)
# set(wxUSE_STATLINE OFF)
# set(wxUSE_STATTEXT OFF)
# set(wxUSE_STATUSBAR OFF)
set(wxUSE_TASKBARBUTTON OFF)
set(wxUSE_TASKBARICON OFF)
set(wxUSE_TOOLBAR_NATIVE OFF)
# set(wxUSE_TEXTCTRL OFF)
set(wxUSE_TIMEPICKCTRL OFF)
set(wxUSE_TIPWINDOW OFF)
set(wxUSE_TOGGLEBTN OFF)
set(wxUSE_TOOLBAR OFF)
set(wxUSE_TOOLBOOK OFF)
set(wxUSE_TREEBOOK OFF)
set(wxUSE_TREECTRL OFF)
set(wxUSE_TREELISTCTRL OFF)

set(wxUSE_COMMON_DIALOGS OFF)
# set(wxUSE_ABOUTDLG OFF)
set(wxUSE_CHOICEDLG OFF)
set(wxUSE_COLOURDLG OFF)
set(wxUSE_CREDENTIALDLG OFF)
set(wxUSE_FILEDLG OFF)
set(wxUSE_FINDREPLDLG OFF)
set(wxUSE_FONTDLG OFF)
set(wxUSE_DIRDLG OFF)
# set(wxUSE_MSGDLG OFF)
set(wxUSE_NUMBERDLG OFF)
set(wxUSE_SPLASH OFF)
set(wxUSE_TEXTDLG OFF)
set(wxUSE_STARTUP_TIPS OFF)
set(wxUSE_PROGRESSDLG OFF)
set(wxUSE_WIZARDDLG OFF)

# set(wxUSE_MENUS OFF)
# set(wxUSE_MENUBAR OFF)
set(wxUSE_MINIFRAME OFF)
# set(wxUSE_TOOLTIPS OFF)
# set(wxUSE_SPLINES OFF)
# set(wxUSE_MOUSEWHEEL OFF)
# set(wxUSE_VALIDATORS OFF)
set(wxUSE_BUSYINFO OFF)
set(wxUSE_HOTKEY OFF)
set(wxUSE_JOYSTICK OFF)
set(wxUSE_METAFILE OFF)
set(wxUSE_DRAGIMAGE OFF)
set(wxUSE_UIACTIONSIMULATOR OFF)
set(wxUSE_DC_TRANSFORM_MATRIX OFF)
# set(wxUSE_WEBVIEW_WEBKIT OFF)
# set(wxUSE_PRIVATE_FONTS OFF)

set(wxUSE_PALETTE OFF)
# set(wxUSE_IMAGE OFF)
set(wxUSE_GIF OFF)
set(wxUSE_PCX OFF)
set(wxUSE_TGA OFF)
set(wxUSE_IFF OFF)
set(wxUSE_PNM OFF)
# set(wxUSE_XPM OFF)
# set(wxUSE_ICO_CUR OFF)

if(WIN32)
  # set(wxUSE_ACCESSIBILITY OFF)
  # set(wxUSE_ACTIVEX OFF)
  # set(wxUSE_CRASHREPORT OFF)
  # set(wxUSE_DC_CACHEING OFF)
  # set(wxUSE_NATIVE_PROGRESSDLG OFF)
  # set(wxUSE_NATIVE_STATUSBAR OFF)
  # set(wxUSE_OWNER_DRAWN OFF)
  # set(wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW OFF)
  # set(wxUSE_TASKBARICON_BALLOONS OFF)
  # set(wxUSE_UXTHEME OFF)
  # set(wxUSE_WEBVIEW_EDGE OFF)
  # set(wxUSE_WEBVIEW_EDGE_STATIC OFF)
  # set(wxUSE_WEBVIEW_IE OFF)
  # set(wxUSE_WINRT OFF)
  # set(wxUSE_WXDIB OFF)
endif()
