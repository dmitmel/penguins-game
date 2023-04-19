#pragma once

#include "bot.h"
#include "game.h"
#include <memory>
#include <wx/dc.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/thread.h>

class GamePanel;
class CanvasPanel;
class BotThread;

class GameController : public wxEvtHandler {
public:
  GameController(GamePanel* panel);
  virtual ~GameController() {}

  void update_game_state_and_indirectly_delete_this();

  virtual void on_activated();
  virtual void configure_bot_turn_ui();
  virtual void configure_log_viewer_ui();
  virtual void on_deactivated(GameController* next_controller);
  virtual void update_tile_attributes();
  virtual void paint_overlay(wxDC& dc);
  virtual void update_status_bar();
  virtual void on_mouse_enter_leave(wxMouseEvent& event);
  virtual void on_mouse_down(wxMouseEvent& event);
  virtual void on_mouse_move(wxMouseEvent& event);
  virtual void on_mouse_up(wxMouseEvent& event);

  GamePanel* panel;
  CanvasPanel* canvas;
  Game* game;
  std::shared_ptr<BotParameters>& bot_params;
};

class PlayerTurnController : public GameController {
public:
  PlayerTurnController(GamePanel* panel) : GameController(panel) {}
  virtual void paint_overlay(wxDC& dc) override;
};

class PlayerPlacementController : public PlayerTurnController {
public:
  PlayerPlacementController(GamePanel* panel) : PlayerTurnController(panel) {}
  virtual void update_tile_attributes() override;
  virtual void on_mouse_move(wxMouseEvent& event) override;
  virtual void on_mouse_up(wxMouseEvent& event) override;
  virtual void update_status_bar() override;
};

class PlayerMovementController : public PlayerTurnController {
public:
  PlayerMovementController(GamePanel* panel) : PlayerTurnController(panel) {}
  virtual void update_tile_attributes() override;
  virtual void paint_overlay(wxDC& dc) override;
  virtual void update_status_bar() override;
  virtual void on_mouse_down(wxMouseEvent& event) override;
  virtual void on_mouse_move(wxMouseEvent& event) override;
  virtual void on_mouse_up(wxMouseEvent& event) override;
};

class BotTurnController : public GameController {
public:
  BotTurnController(GamePanel* panel) : GameController(panel) {}
  virtual ~BotTurnController();
  virtual void configure_bot_turn_ui() override;
  virtual void on_activated() override;
  virtual void on_deactivated(GameController* next_controller) override;
  virtual void update_tile_attributes() override;
  virtual void update_status_bar() override;
  virtual void on_mouse_up(wxMouseEvent& event) override;

  virtual BotThread* create_bot_thread() = 0;
  void on_bot_thread_done_work(bool cancelled);
  void start_bot_thread();
  void stop_bot_thread();
  void unregister_bot_thread(BotThread* thread);

  bool executing_bot_turn = false;
  wxCriticalSection bot_thread_cs;
  BotThread* bot_thread = nullptr;
};

class BotPlacementController : public BotTurnController {
public:
  BotPlacementController(GamePanel* panel) : BotTurnController(panel) {}
  virtual BotThread* create_bot_thread() override;
};

class BotMovementController : public BotTurnController {
public:
  BotMovementController(GamePanel* panel) : BotTurnController(panel) {}
  virtual BotThread* create_bot_thread() override;
};

class GameEndedController : public GameController {
public:
  GameEndedController(GamePanel* panel) : GameController(panel) {}
  virtual void on_activated() override;
  virtual void update_status_bar() override;
};

class LogEntryViewerController : public GameController {
public:
  LogEntryViewerController(GamePanel* panel, size_t entry_index)
  : GameController(panel), entry_index(entry_index) {}
  virtual ~LogEntryViewerController() {}
  virtual void on_activated() override;
  virtual void update_status_bar() override;
  virtual void configure_log_viewer_ui() override;
  virtual void paint_overlay(wxDC& dc) override;
  size_t entry_index;
};
