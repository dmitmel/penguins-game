#pragma once

#include "gui/game.hh"
#include "gui/tileset.hh"
#include <wx/app.h>

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

  TilesetHelper tileset{ CanvasPanel::TILE_SIZE / TilesetHelper::TILE_SIZE };
  GameFrame* game_frame;
};

wxDECLARE_APP(PenguinsApp);
