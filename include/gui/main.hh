#pragma once

#include "gui/tileset.hh"
#include <wx/app.h>
#include <wx/iconbndl.h>

class GameFrame;

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

  wxIconBundle app_icon;
  TilesetHelper tileset;
  GameFrame* game_frame;
};

wxDECLARE_APP(PenguinsApp);
