#pragma once

#include "game.h"
#include <memory>
#include <wx/defs.h>
#include <wx/string.h>

class GuiGameState {
  wxDECLARE_NO_COPY_CLASS(GuiGameState);

public:
  GuiGameState() {}

  bool game_ended = false;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::unique_ptr<wxString[]> player_names{ nullptr };
};
