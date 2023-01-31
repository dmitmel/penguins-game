#include "gui/main.hh"
#include "gui/game.hh"
#include "random.h"
#include <wx/app.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/stdpaths.h>

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

  this->game_frame = new GameFrame(nullptr, wxID_ANY, this->game_state);
  this->game_frame->Centre();
  this->game_frame->Show();

  wxYieldIfNeeded();
  this->game_frame->start_new_game();

  return true;
}
