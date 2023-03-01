#pragma once

#include "bot.h"
#include "game.h"
#include <memory>
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/vector.h>

enum BoardGenType {
  BOARD_GEN_RANDOM,
  BOARD_GEN_ISLAND,
  BOARD_GEN_MAX,
};

enum PlayerType {
  PLAYER_NORMAL,
  PLAYER_BOT,
  PLAYER_TYPE_MAX,
};

class GuiGameState {
public:
  GuiGameState() {}

  bool game_ended = false;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::shared_ptr<BotParameters> bot_params{ nullptr };
  wxVector<wxString> player_names;
  wxVector<PlayerType> player_types;

  bool current_player_type_is(PlayerType expected) const {
    Game* game = this->game.get();
    if (game_check_player_index(game, game->current_player_index)) {
      return this->player_types.at(game->current_player_index) == expected;
    }
    return false;
  }
};
