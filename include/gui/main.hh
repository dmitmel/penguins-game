#pragma once

#include "gui/game.hh"
#include "gui/tileset.hh"
#include "utils.h"
#include <memory>
#include <wx/app.h>

static_assert(
  std::is_standard_layout<wxPoint>() && std::is_standard_layout<Coords>() &&
    sizeof(wxPoint) == sizeof(Coords) && sizeof(wxPoint::x) == sizeof(Coords::x) &&
    offsetof(wxPoint, x) == offsetof(Coords, x) && sizeof(wxPoint::y) == sizeof(Coords::y) &&
    offsetof(wxPoint, y) == offsetof(Coords, y),
  "The layout of wxPoint and Coords must be compatible"
);

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

protected:
  GameFrame* game_frame;
  GuiGameState game_state;
  TilesetHelper tileset{ CanvasPanel::CELL_SIZE / TilesetHelper::TILE_SIZE };
};

wxDECLARE_APP(PenguinsApp);
