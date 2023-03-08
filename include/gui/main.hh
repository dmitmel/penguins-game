#pragma once

#include "gui/tileset.hh"
#include <wx/app.h>

class GameFrame;

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

  TilesetHelper tileset;
  GameFrame* game_frame;
};

wxDECLARE_APP(PenguinsApp);
