#include "gui/main.hh"
#include "gui/game.hh"
#include "random.h"

wxIMPLEMENT_APP(PenguinsApp);

bool PenguinsApp::OnInit() {
  random_init();
  wxImage::AddHandler(new wxPNGHandler());
  tileset.load();

  this->game_frame = new GameFrame(nullptr, wxID_ANY, this->game_state, this->tileset);
  this->game_frame->Centre();
  this->game_frame->Show();

  this->Yield(true);
  this->game_frame->start_new_game();

  return true;
}
