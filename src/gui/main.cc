#include "gui/main.hh"
#include "gui/game.hh"
#include "random.h"
#include "resources_appicon_256_png.h"
#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/version.h>

wxIMPLEMENT_APP(PenguinsApp);

bool PenguinsApp::OnInit() {
  if (!wxApp::OnInit()) return false;

  this->SetAppName("penguins-game-gui");
#if wxCHECK_VERSION(3, 1, 1)
  wxStandardPaths::Get().SetFileLayout(wxStandardPathsBase::FileLayout_XDG);
#endif
  random_init();
  wxImage::AddHandler(new wxPNGHandler());
  tileset.load();

  wxMemoryInputStream icon_stream(resources_appicon_256_png, resources_appicon_256_png_size);
  // The triple conversion necessary to load the icon here is... meh.
  this->app_icon.CopyFromBitmap(wxBitmap(wxImage(icon_stream, wxBITMAP_TYPE_PNG)));

  this->game_frame = new GameFrame(nullptr, wxID_ANY);
  this->game_frame->Centre();
  this->game_frame->Show();

  wxYieldIfNeeded();

  this->game_frame->start_new_game();

  return true;
}
