#pragma once

#include "game.h"
#include "gui/game_state.hh"
#include <wx/dc.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/thread.h>

class GameFrame;
class CanvasPanel;
class BotThread;

class GameController {
public:
  GameController(GameFrame* game_frame);
  virtual ~GameController() {}

  void update_game_state_and_indirectly_delete_this();

  virtual void on_activated();
  virtual void update_tile_attributes();
  virtual void paint_overlay(wxDC& WXUNUSED(dc)) {}
  virtual void on_mouse_down(wxMouseEvent& WXUNUSED(event)) {}
  virtual void on_mouse_move(wxMouseEvent& WXUNUSED(event)) {}
  virtual void on_mouse_up(wxMouseEvent& WXUNUSED(event)) {}

  GameFrame* game_frame;
  CanvasPanel* canvas;
  GuiGameState& state;
  Game* game;
};

class PlayerTurnController : public GameController {
public:
  PlayerTurnController(GameFrame* game_frame) : GameController(game_frame) {}
  virtual void paint_overlay(wxDC& dc) override;
};

class PlayerPlacementController : public PlayerTurnController {
public:
  PlayerPlacementController(GameFrame* game_frame) : PlayerTurnController(game_frame) {}
  virtual void update_tile_attributes() override;
  virtual void on_mouse_move(wxMouseEvent& event) override;
  virtual void on_mouse_up(wxMouseEvent& event) override;
};

class PlayerMovementController : public PlayerTurnController {
public:
  PlayerMovementController(GameFrame* game_frame) : PlayerTurnController(game_frame) {}
  virtual void update_tile_attributes() override;
  virtual void paint_overlay(wxDC& dc) override;
  virtual void on_mouse_down(wxMouseEvent& event) override;
  virtual void on_mouse_move(wxMouseEvent& event) override;
  virtual void on_mouse_up(wxMouseEvent& event) override;
};

class BotTurnController : public GameController {
public:
  BotTurnController(GameFrame* game_frame) : GameController(game_frame) {}
  virtual ~BotTurnController();
  virtual void on_activated() override;
  virtual void update_tile_attributes() override;
  virtual void on_mouse_up(wxMouseEvent& event) override;

  void unregister_bot_thread(BotThread* thread);
  void on_bot_thread_done_work(bool cancelled);
  void run_bot_thread(BotThread* thread);
  void stop_bot_thread();

  bool executing_bot_turn = false;
  wxCriticalSection bot_thread_cs;
  BotThread* bot_thread = nullptr;
};

class BotPlacementController : public BotTurnController {
public:
  BotPlacementController(GameFrame* game_frame) : BotTurnController(game_frame) {}
  virtual void on_activated() override;
};

class BotMovementController : public BotTurnController {
public:
  BotMovementController(GameFrame* game_frame) : BotTurnController(game_frame) {}
  virtual void on_activated() override;
};

class GameEndedController : public GameController {
public:
  GameEndedController(GameFrame* game_frame) : GameController(game_frame) {}
};