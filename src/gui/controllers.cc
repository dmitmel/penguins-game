#include "gui/controllers.hh"
#include "board.h"
#include "game.h"
#include "gui/bot_thread.hh"
#include "gui/canvas.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <memory>
#include <wx/button.h>
#include <wx/debug.h>
#include <wx/defs.h>

GameController::GameController(GameFrame* game_frame)
: game_frame(game_frame)
, canvas(game_frame->canvas)
, state(game_frame->state)
, game(state.game.get()) {}

void GameController::update_game_state_and_indirectly_delete_this() {
  this->game_frame->update_game_state();
}

void GameController::on_activated() {
  this->configure_bot_turn_ui();
  this->configure_log_viewer_ui();
}

void GameController::configure_bot_turn_ui() {
  this->game_frame->stop_bot_progress();
}

void GameController::configure_log_viewer_ui() {
  this->game_frame->show_current_turn_btn->Disable();
}

void GameController::on_deactivated(GameController* WXUNUSED(next_controller)) {}
void GameController::paint_overlay(wxDC& WXUNUSED(dc)) {}
void GameController::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {}
void GameController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {}
void GameController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {}

void GameEndedController::on_activated() {
  this->GameController::on_activated();
  if (!this->state.game_ended) {
    this->state.game_ended = true;
    this->game_frame->CallAfter(&GameFrame::end_game);
  }
}

void BotTurnController::configure_bot_turn_ui() {
  this->game_frame->start_bot_progress();
}

void BotTurnController::on_deactivated(GameController* WXUNUSED(next_controller)) {
  this->stop_bot_thread();
}

void BotPlacementController::on_activated() {
  this->BotTurnController::on_activated();
  wxASSERT(!this->executing_bot_turn);
  this->executing_bot_turn = true;
  this->run_bot_thread(new BotPlacementThread(this));
}

void BotMovementController::on_activated() {
  this->BotTurnController::on_activated();
  wxASSERT(!this->executing_bot_turn);
  this->executing_bot_turn = true;
  this->run_bot_thread(new BotMovementThread(this));
}

BotTurnController::~BotTurnController() {
  this->stop_bot_thread();
  this->executing_bot_turn = false;
}

void BotTurnController::on_bot_thread_done_work(bool cancelled) {
  this->executing_bot_turn = false;
  if (!cancelled) {
    this->update_game_state_and_indirectly_delete_this();
  } else {
    this->game_frame->stop_bot_progress();
  }
}

void BotTurnController::run_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::IsMain());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  wxASSERT(this->bot_thread == nullptr);
  this->bot_thread = thread;
  wxThreadError code WX_ATTRIBUTE_UNUSED = thread->Run();
  wxASSERT(code == wxTHREAD_NO_ERROR);
}

void BotTurnController::stop_bot_thread() {
  wxASSERT(wxThread::IsMain());
  // Well, this ain't the best way of doing thread synchronization, but it sure
  // does work.
  std::shared_ptr<BotThreadShared> shared = nullptr;
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    if (this->bot_thread) {
      this->bot_thread->cancel();
      shared = this->bot_thread->shared;
      this->bot_thread = nullptr;
    }
  }
  if (shared) {
    shared->wait_for_exit();
  }
}

void BotTurnController::unregister_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::GetCurrentId() == thread->GetId());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  // Another bot thread might have just been spun up after cancelling this one,
  // so check that we are unregistering the correct one to be sure.
  if (this->bot_thread == thread) {
    this->bot_thread = nullptr;
  }
}

void LogEntryViewerController::on_activated() {
  this->GameController::on_activated();
  const GameLogEntry* entry = game_get_log_entry(game, this->entry_index);
  size_t adjusted_index = this->entry_index;
  if (entry->type == GAME_LOG_ENTRY_PHASE_CHANGE) {
    if (entry->data.phase_change.new_phase == GAME_PHASE_END) {
      adjusted_index = game->log_length;
    }
  } else if (entry->type == GAME_LOG_ENTRY_PLACEMENT) {
    adjusted_index += 1;
  }
  game_rewind_state_to_log_entry(game, adjusted_index);
}

void LogEntryViewerController::configure_log_viewer_ui() {
  this->game_frame->show_current_turn_btn->Enable();
}

void GameController::update_tile_attributes() {
  set_all_tiles_attr(game, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
}

void PlayerPlacementController::update_tile_attributes() {
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  bool is_a_tile_selected =
    this->canvas->mouse_within_window && is_tile_in_bounds(game, curr_coords);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      bool blocked = !validate_placement_simple(game, coords);
      set_tile_attr(game, coords, TILE_BLOCKED_FOR_CURSOR, blocked);
      set_tile_attr(game, coords, TILE_BLOCKED, blocked && is_a_tile_selected);
    }
  }
}

void PlayerMovementController::update_tile_attributes() {
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (!is_tile_in_bounds(game, selected_penguin)) {
    int current_player_id = game_get_current_player(game)->id;
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        int tile = get_tile(game, coords);
        bool blocked = get_tile_player_id(tile) != current_player_id;
        set_tile_attr(game, coords, TILE_BLOCKED_FOR_CURSOR, blocked);
        set_tile_attr(game, coords, TILE_BLOCKED, false);
      }
    }
    return;
  }
  // A penguin is selected
  set_all_tiles_attr(game, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, true);
  PossibleSteps moves = calculate_penguin_possible_moves(game, selected_penguin);
  bool any_steps = false;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords coords = selected_penguin;
    Coords d = DIRECTION_TO_COORDS[dir];
    any_steps = any_steps || moves.steps[dir] != 0;
    for (int steps = moves.steps[dir]; steps > 0; steps--) {
      coords.x += d.x, coords.y += d.y;
      set_tile_attr(game, coords, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
    }
  }
  set_tile_attr(game, selected_penguin, TILE_BLOCKED, false);
  if (any_steps && !this->canvas->mouse_is_down) {
    set_tile_attr(game, selected_penguin, TILE_BLOCKED_FOR_CURSOR, false);
  }
}

void BotTurnController::update_tile_attributes() {
  set_all_tiles_attr(game, TILE_BLOCKED, false);
  set_all_tiles_attr(game, TILE_BLOCKED_FOR_CURSOR, true);
}

void PlayerTurnController::paint_overlay(wxDC& dc) {
  Coords current_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, current_coords) && this->canvas->mouse_within_window) {
    bool blocked = get_tile_attr(game, current_coords, TILE_BLOCKED_FOR_CURSOR);
    this->canvas->paint_selected_tile_outline(dc, current_coords, blocked);
  }
}

void PlayerMovementController::paint_overlay(wxDC& dc) {
  this->PlayerTurnController::paint_overlay(dc);

  if (!this->canvas->mouse_is_down) return;
  Coords penguin = this->canvas->get_selected_penguin_coords();
  Coords target = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (!(is_tile_in_bounds(game, penguin) && is_tile_in_bounds(game, target))) return;
  if (coords_same(penguin, target)) return;

  Coords move_fail = penguin;
  MovementError result = validate_movement(game, penguin, target, &move_fail);
  this->canvas->paint_move_arrow(dc, penguin, target, move_fail, result == VALID_INPUT);
}

void LogEntryViewerController::paint_overlay(wxDC& dc) {
  const GameLogEntry* entry = game_get_log_entry(this->state.game.get(), this->entry_index);
  if (entry->type == GAME_LOG_ENTRY_PLACEMENT) {
    this->canvas->paint_selected_tile_outline(dc, entry->data.placement.target);
  } else if (entry->type == GAME_LOG_ENTRY_MOVEMENT) {
    this->canvas->paint_move_arrow(dc, entry->data.movement.penguin, entry->data.movement.target);
  }
}

void PlayerMovementController::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  this->canvas->Refresh();
}

void PlayerMovementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (is_tile_in_bounds(game, selected_penguin)) {
    set_tile_attr(game, selected_penguin, TILE_DIRTY, true);
  }
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && coords_same(prev_coords, curr_coords)) {
    if (validate_placement(game, curr_coords) == PLACEMENT_VALID) {
      place_penguin(game, curr_coords);
      this->update_game_state_and_indirectly_delete_this();
      return;
    }
  }
}

void PlayerMovementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && is_tile_in_bounds(game, prev_coords)) {
    if (coords_same(prev_coords, curr_coords) && validate_movement_start(game, curr_coords)) {
      // This is a hacky way of doing what I want: when the user has simply
      // clicked on a penguin, let them then move it without dragging the mouse
      // all the way.
      this->canvas->mouse_is_down = true;
    } else if (validate_movement(game, prev_coords, curr_coords, nullptr) == VALID_INPUT) {
      move_penguin(game, prev_coords, curr_coords);
      this->update_game_state_and_indirectly_delete_this();
      return;
    }
  }
  this->canvas->Refresh();
}

void BotTurnController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  if (!this->executing_bot_turn) {
    this->on_activated();
  }
}
