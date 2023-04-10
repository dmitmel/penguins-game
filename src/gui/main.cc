#include "gui/main.hh"
#include "gui/game.hh"
#include "resources_appicon_16_png.h"
#include "resources_appicon_256_png.h"
#include "resources_appicon_64_png.h"
#include <wx/app.h>
#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/iconbndl.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/version.h>

wxIMPLEMENT_APP(PenguinsApp);

bool PenguinsApp::OnInit() {
  if (!wxApp::OnInit()) return false;

  this->SetAppName("penguins-game-gui");
#if wxCHECK_VERSION(3, 1, 1)
  wxStandardPaths::Get().SetFileLayout(wxStandardPathsBase::FileLayout_XDG);
#endif
  wxImage::AddHandler(new wxPNGHandler());
  tileset.load();

  auto add_icon = [](wxIconBundle& bundle, const void* data, size_t size) {
    wxMemoryInputStream input_stream(data, size);
    bundle.AddIcon(input_stream, wxBITMAP_TYPE_PNG);
  };
  add_icon(this->app_icon, resources_appicon_16_png, resources_appicon_16_png_size);
  add_icon(this->app_icon, resources_appicon_64_png, resources_appicon_64_png_size);
  add_icon(this->app_icon, resources_appicon_256_png, resources_appicon_256_png_size);

  this->game_frame = new GameFrame(nullptr, wxID_ANY);
  this->game_frame->Centre();
  this->game_frame->Show();

  return true;
}
