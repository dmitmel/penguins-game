#include "gui/game.hh"
#include "board.h"
#include "bot.h"
#include "game.h"
#include "gui/bot_thread.hh"
#include "gui/game_end_dialog.hh"
#include "gui/game_state.hh"
#include "gui/main.hh"
#include "gui/new_game_dialog.hh"
#include "gui/player_info_box.hh"
#include "gui/tileset.hh"
#include "movement.h"
#include "placement.h"
#include "resources_appicon_256_png.h"
#include "utils.h"
#include <cassert>
#include <memory>
#include <wx/anybutton.h>
#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/panel.h>
#include <wx/pen.h>
#include <wx/persist.h>
#include <wx/region.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/thread.h>
#include <wx/timer.h>
#include <wx/types.h>
#include <wx/utils.h>
#include <wx/window.h>

static inline bool coords_same(const Coords& a, const Coords& b) {
  return a.x == b.x && a.y == b.y;
}

GameFrame::GameFrame(wxWindow* parent, wxWindowID id) : wxFrame(parent, id, "Penguins game") {
  this->Bind(wxEVT_DESTROY, &GameFrame::on_destroy, this);

  wxMemoryInputStream icon_stream(resources_appicon_256_png, resources_appicon_256_png_size);
  wxIcon app_icon;
  // The triple conversion necessary to load the icon here is... meh.
  app_icon.CopyFromBitmap(wxBitmap(wxImage(icon_stream, wxBITMAP_TYPE_PNG)));
  this->SetIcon(app_icon);

  this->root_panel = new wxPanel(this, wxID_ANY);

  auto menu_bar = new wxMenuBar();
  wxMenuItem* item;

  auto menu_file = new wxMenu();
  menu_bar->Append(menu_file, "&File");

  item = menu_file->Append(wxID_ANY, "&New game\tCtrl-N", "Start a new game");
  this->Bind(wxEVT_MENU, &GameFrame::on_new_game, this, item->GetId());

  item = menu_file->Append(wxID_ANY, "&Close game\tCtrl-W", "Close the current game");
  this->Bind(wxEVT_MENU, &GameFrame::on_close_game, this, item->GetId());

  menu_file->AppendSeparator();

  item = menu_file->Append(wxID_EXIT);
  this->Bind(wxEVT_MENU, &GameFrame::on_exit, this, item->GetId());

  auto menu_help = new wxMenu();
  menu_bar->Append(menu_help, "&Help");

  item = menu_help->Append(wxID_ABOUT);
  this->Bind(wxEVT_MENU, &GameFrame::on_about, this, item->GetId());

  this->SetMenuBar(menu_bar);

  this->CreateStatusBar(2);
  this->SetStatusText("Welcome to wxWidgets!");
  int status_bar_widths[2] = { -3, -1 };
  this->SetStatusWidths(2, status_bar_widths);

  this->progress_container = new wxWindow(this->GetStatusBar(), wxID_ANY);
  this->progress_container->Hide();

  this->progress_bar = new wxGauge(
    this->progress_container,
    wxID_ANY,
    100,
    wxDefaultPosition,
    wxSize(200, -1),
    wxGA_HORIZONTAL | wxGA_SMOOTH
  );

  this->stop_bot_button = new wxButton(
    this->progress_container,
    wxID_STOP,
    wxEmptyString,
    wxDefaultPosition,
    wxDefaultSize,
    wxBORDER_NONE | wxBU_EXACTFIT
  );
  this->stop_bot_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) -> void {
    this->stop_bot_thread();
  });

  auto progress_hbox = new wxBoxSizer(wxHORIZONTAL);
  progress_hbox->AddStretchSpacer(1);
  progress_hbox->Add(this->progress_bar, wxSizerFlags(1).Centre().HorzBorder());
  progress_hbox->Add(this->stop_bot_button, wxSizerFlags().Centre().HorzBorder());
  this->progress_container->SetSizer(progress_hbox);

  this->GetStatusBar()->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) -> void {
    event.Skip();
    wxRect rect;
    if (this->GetStatusBar()->GetFieldRect(1, rect)) {
      // This sets both the position and the size at the same time.
      this->progress_container->SetSize(rect);
    }
  });

  this->progress_timer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) -> void {
    this->progress_container->Show();
    this->progress_bar->Pulse();
  });

  this->scrolled_panel = new wxScrolledWindow(this->root_panel, wxID_ANY);
  auto scroll_vbox = new wxBoxSizer(wxVERTICAL);
  auto scroll_hbox = new wxBoxSizer(wxHORIZONTAL);
  this->scrolled_panel->SetScrollRate(10, 10);
  wxSize default_board_size(
    NewGameDialog::DEFAULT_BOARD_WIDTH, NewGameDialog::DEFAULT_BOARD_HEIGHT
  );
  this->empty_canvas_panel = new wxPanel(
    this->scrolled_panel, wxID_ANY, wxDefaultPosition, default_board_size * CanvasPanel::TILE_SIZE
  );
  scroll_hbox->Add(this->empty_canvas_panel, wxSizerFlags(1).Centre());
  this->canvas_sizer = scroll_hbox;
  scroll_vbox->Add(scroll_hbox, wxSizerFlags(1).Centre());
  this->scrolled_panel->SetSizer(scroll_vbox);

  auto panel_vbox = new wxBoxSizer(wxVERTICAL);

  this->players_box = new wxBoxSizer(wxHORIZONTAL);
  panel_vbox->Add(players_box, wxSizerFlags().Centre().Border(wxALL & ~wxDOWN));

  auto canvas_hbox = new wxBoxSizer(wxHORIZONTAL);
  canvas_hbox->Add(this->scrolled_panel, wxSizerFlags(1).Expand().Border());
  panel_vbox->Add(canvas_hbox, wxSizerFlags(1).Expand().Border());

  this->root_panel->SetSizer(panel_vbox);

  auto root_vbox = new wxBoxSizer(wxVERTICAL);
  root_vbox->Add(this->root_panel, wxSizerFlags(1).Expand());
  this->SetSizer(root_vbox);
  this->update_layout();
}

void GameFrame::update_layout() {
  this->Layout();
  this->scrolled_panel->GetSizer()->SetSizeHints(this->scrolled_panel);
  this->GetSizer()->SetSizeHints(this);
}

GameFrame::~GameFrame() {
  this->SendDestroyEvent();
}

void GameFrame::on_destroy(wxWindowDestroyEvent& WXUNUSED(event)) {
  this->stop_bot_thread();
}

void GameFrame::on_new_game(wxCommandEvent& WXUNUSED(event)) {
  this->start_new_game();
}

void GameFrame::on_close_game(wxCommandEvent& WXUNUSED(event)) {
  this->close_game();
  this->update_layout();
  this->Refresh();
  this->Centre();
}

void GameFrame::on_exit(wxCommandEvent& WXUNUSED(event)) {
  this->Close(true);
}

void GameFrame::on_about(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION, this
  );
}

void GameFrame::start_new_game() {
  std::unique_ptr<NewGameDialog> dialog(new NewGameDialog(this, wxID_ANY));
  wxPersistentRegisterAndRestore(dialog.get());
  int result = dialog->ShowModal();
  if (result != wxID_OK) {
    return;
  }

  this->close_game();

  Game* game = game_new();
  this->state.game.reset(game);
  this->state.player_names.reset(new wxString[dialog->get_number_of_players()]);
  this->state.player_types.reset(new PlayerType[dialog->get_number_of_players()]);

  this->state.bot_params.reset(new BotParameters);
  init_bot_parameters(this->state.bot_params.get());

  game_begin_setup(game);
  game_set_penguins_per_player(game, dialog->get_penguins_per_player());
  game_set_players_count(game, dialog->get_number_of_players());
  for (int i = 0; i < game->players_count; i++) {
    game_set_player_name(game, i, dialog->get_player_name(i).c_str());
    this->state.player_names[i] = dialog->get_player_name(i);
    this->state.player_types[i] = dialog->get_player_type(i);
  }
  setup_board(game, dialog->get_board_width(), dialog->get_board_height());
  switch (dialog->get_board_gen_type()) {
    case BOARD_GEN_RANDOM: generate_board_random(game); break;
    case BOARD_GEN_ISLAND: generate_board_island(game); break;
    default: break;
  }
  game_end_setup(game);

  this->canvas_panel = new CanvasPanel(this->scrolled_panel, wxID_ANY, this);
  bool replaced = this->canvas_sizer->Replace(this->empty_canvas_panel, this->canvas_panel);
  wxASSERT(replaced);
  this->empty_canvas_panel->Hide();

  this->canvas_panel->tile_attributes.reset(new wxByte[game->board_width * game->board_height]);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      *this->canvas_panel->tile_attrs_ptr(coords) = TILE_DIRTY | TILE_BLOCKED_DIRTY;
    }
  }

  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes.reset(new PlayerInfoBox*[game->players_count]);
  for (int i = 0; i < game->players_count; i++) {
    auto player_box = new PlayerInfoBox(this->root_panel, wxID_ANY);
    this->players_box->Add(
      player_box->GetContainingSizer(), wxSizerFlags().Border(wxALL & ~wxDOWN)
    );
    this->player_info_boxes[i] = player_box;
  }
  this->update_player_info_boxes();

  this->update_layout();
  this->Refresh();
  this->Centre();

  this->update_game_state();
}

void GameFrame::update_game_state() {
  Game* game = this->state.game.get();
  game_advance_state(game);

  // Repaint
  this->update_player_info_boxes();
  this->canvas_panel->Refresh();

  if (this->get_current_player_type() == PLAYER_BOT) {
    this->start_bot_progress();
    this->execute_bot_turn();
  } else {
    this->stop_bot_progress();
  }

  if (game->phase == GAME_PHASE_END) {
    if (!this->state.game_ended) {
      this->state.game_ended = true;
      this->CallAfter(&GameFrame::end_game);
    }
  }
}

void GameFrame::execute_bot_turn() {
  wxASSERT(this->get_current_player_type() == PLAYER_BOT);
  wxASSERT(!this->executing_bot_turn);
  this->executing_bot_turn = true;
  Game* game = this->state.game.get();
  if (game->phase == GAME_PHASE_PLACEMENT) {
    this->run_bot_thread(new BotPlacementThread(this));
  } else if (game->phase == GAME_PHASE_MOVEMENT) {
    this->run_bot_thread(new BotMovementThread(this));
  } else {
    wxASSERT_MSG(false, "cannot execute a bot turn in the current game phase");
  }
}

void GameFrame::on_bot_thread_done_work(bool cancelled) {
  // Wait for the bot thread to stop completely.
  this->stop_bot_thread();
  this->executing_bot_turn = false;
  if (!cancelled) {
    this->update_game_state();
  } else {
    this->stop_bot_progress();
  }
}

void GameFrame::run_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::IsMain());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  wxASSERT(this->bot_thread == nullptr);
  this->bot_thread = thread;
  wxASSERT(thread->Run() == wxTHREAD_NO_ERROR);
}

void GameFrame::stop_bot_thread() {
  wxASSERT(wxThread::IsMain());
  // Well, this ain't the best way of doing thread synchronization, but it sure
  // does work.
  std::shared_ptr<wxSemaphore> exit_semaphore = nullptr;
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    if (this->bot_thread) {
      this->bot_thread->cancel();
      exit_semaphore = this->bot_thread->exit_semaphore;
      this->bot_thread = nullptr;
    }
  }
  if (exit_semaphore) {
    exit_semaphore->Wait();
  }
}

void GameFrame::on_bot_thread_exited(BotThread* thread) {
  wxASSERT(wxThread::GetCurrentId() == thread->GetId());
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    // Another bot thread might have just been spun up after cancelling this one,
    // so check that we are unregistering the correct one to be sure.
    if (this->bot_thread == thread) {
      this->bot_thread = nullptr;
    }
  }
  thread->exit_semaphore->Post();
}

void GameFrame::start_bot_progress() {
  // The progress_container is shown by the timer.
  if (!this->progress_timer.IsRunning()) {
    this->progress_timer.Start(50);
  }
  if (!this->busy_cursor_changer) {
    this->busy_cursor_changer.reset(new wxBusyCursor());
  }
}

void GameFrame::stop_bot_progress() {
  this->progress_container->Hide();
  this->progress_timer.Stop();
  this->busy_cursor_changer.reset(nullptr);
}

PlayerType GameFrame::get_current_player_type() const {
  Game* game = this->state.game.get();
  if (game_check_player_index(game, game->current_player_index)) {
    return this->state.player_types[game->current_player_index];
  } else {
    return PLAYER_TYPE_MAX;
  }
}

void GameFrame::place_penguin(Coords target) {
  ::place_penguin(this->state.game.get(), target);
  this->canvas_panel->set_tile_attr(target, TILE_DIRTY, true);
  this->canvas_panel->set_tile_neighbors_attr(target, TILE_DIRTY, true);
}

void GameFrame::move_penguin(Coords penguin, Coords target) {
  ::move_penguin(this->state.game.get(), penguin, target);
  this->canvas_panel->set_tile_attr(penguin, TILE_DIRTY, true);
  this->canvas_panel->set_tile_neighbors_attr(penguin, TILE_DIRTY, true);
  this->canvas_panel->set_tile_attr(target, TILE_DIRTY, true);
  this->canvas_panel->set_tile_neighbors_attr(target, TILE_DIRTY, true);
}

void GameFrame::end_game() {
  std::unique_ptr<GameEndDialog> dialog(
    new GameEndDialog(this, wxID_ANY, this->state.game.get(), this->state.player_names.get())
  );
  dialog->ShowModal();
}

void GameFrame::close_game() {
  if (this->canvas_panel) {
    bool replaced = this->canvas_sizer->Replace(this->canvas_panel, this->empty_canvas_panel);
    wxASSERT(replaced);
    this->canvas_panel->Destroy();
    this->canvas_panel = nullptr;
    this->empty_canvas_panel->Show();
  }
  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes.reset(nullptr);
  this->state = GuiGameState();
}

void GameFrame::update_player_info_boxes() {
  Game* game = this->state.game.get();
  for (int i = 0; i < game->players_count; i++) {
    this->player_info_boxes[i]->update_data(game, i, this->state.player_names[i]);
    this->player_info_boxes[i]->Refresh();
  }
}

// clang-format off
wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
  EVT_PAINT(CanvasPanel::on_paint)
  EVT_MOUSE_EVENTS(CanvasPanel::on_any_mouse_event)
wxEND_EVENT_TABLE();
// clang-format on

CanvasPanel::CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame)
: wxPanel(parent, id), game_frame(game_frame), game(game_frame->state.game.get()) {
  this->SetInitialSize(this->get_canvas_size());
#ifdef __WXMSW__
  // Necessary to avoid flicker on Windows, see <https://wiki.wxwidgets.org/Flicker-Free_Drawing>.
  this->SetDoubleBuffered(true);
#endif
}

wxSize CanvasPanel::get_canvas_size() const {
  return TILE_SIZE * wxSize(game->board_width, game->board_height);
}

Coords CanvasPanel::tile_coords_at_point(wxPoint point) const {
  return { point.x / TILE_SIZE, point.y / TILE_SIZE };
}

wxRect CanvasPanel::get_tile_rect(Coords coords) const {
  return wxRect(coords.x * TILE_SIZE, coords.y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

wxPoint CanvasPanel::get_tile_centre(Coords coords) const {
  wxRect rect = this->get_tile_rect(coords);
  return rect.GetPosition() + rect.GetSize() / 2;
}

Coords CanvasPanel::get_selected_penguin_coords() const {
  Coords null_coords = { -1, -1 };
  int player_idx = game->current_player_index;
  if (!game_check_player_index(game, player_idx)) return null_coords;
  int player_id = game_get_player(game, player_idx)->id;
  wxPoint curr_tile_pos = this->mouse_is_down ? this->mouse_drag_pos : this->mouse_pos;
  Coords curr_coords = this->tile_coords_at_point(curr_tile_pos);
  if (!is_tile_in_bounds(game, curr_coords)) return null_coords;
  int tile = get_tile(game, curr_coords);
  return get_tile_player_id(tile) == player_id ? curr_coords : null_coords;
}

wxByte* CanvasPanel::tile_attrs_ptr(Coords coords) const {
  assert(is_tile_in_bounds(game, coords));
  return &this->tile_attributes[coords.x + coords.y * game->board_width];
}

void CanvasPanel::set_tile_attr(Coords coords, wxByte attr, bool value) {
  wxByte* tile_attrs = this->tile_attrs_ptr(coords);
  *tile_attrs = (*tile_attrs & ~attr) | (value ? attr : 0);
}

void CanvasPanel::set_tile_neighbors_attr(Coords coords, wxByte attr, bool value) {
  for (int dir = 0; dir < NEIGHBOR_MAX; dir++) {
    Coords neighbor = NEIGHBOR_TO_COORDS[dir];
    neighbor.x += coords.x, neighbor.y += coords.y;
    if (!is_tile_in_bounds(game, neighbor)) continue;
    this->set_tile_attr(neighbor, attr, value);
  }
}

void CanvasPanel::set_all_tiles_attr(wxByte attr, bool value) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      this->set_tile_attr(coords, attr, value);
    }
  }
}

void CanvasPanel::update_blocked_tiles() {
  if (this->game_frame->get_current_player_type() != PLAYER_NORMAL) {
    this->set_all_tiles_attr(TILE_BLOCKED, false);
    this->set_all_tiles_attr(TILE_BLOCKED_FOR_CURSOR, true);
  } else if (game->phase == GAME_PHASE_PLACEMENT) {
    Coords curr_coords = this->tile_coords_at_point(this->mouse_pos);
    bool is_a_tile_selected = this->mouse_within_window && is_tile_in_bounds(game, curr_coords);
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        bool blocked = !validate_placement_simple(game, coords);
        this->set_tile_attr(coords, TILE_BLOCKED_FOR_CURSOR, blocked);
        this->set_tile_attr(coords, TILE_BLOCKED, blocked && is_a_tile_selected);
      }
    }
  } else if (game->phase == GAME_PHASE_MOVEMENT) {
    Coords selected_penguin = this->get_selected_penguin_coords();
    if (this->mouse_within_window && is_tile_in_bounds(game, selected_penguin)) {
      // A penguin is selected
      for (int y = 0; y < game->board_height; y++) {
        for (int x = 0; x < game->board_width; x++) {
          Coords coords = { x, y };
          this->set_tile_attr(coords, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, true);
        }
      }
      PossibleSteps moves = calculate_penguin_possible_moves(game, selected_penguin);
      int steps_sum = 0;
      for (int dir = 0; dir < DIRECTION_MAX; dir++) {
        Coords coords = selected_penguin;
        Coords d = DIRECTION_TO_COORDS[dir];
        steps_sum += moves.steps[dir];
        for (int steps = moves.steps[dir]; steps > 0; steps--) {
          coords.x += d.x, coords.y += d.y;
          this->set_tile_attr(coords, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
        }
      }
      this->set_tile_attr(selected_penguin, TILE_BLOCKED, false);
      if (steps_sum != 0 && !this->mouse_is_down) {
        this->set_tile_attr(selected_penguin, TILE_BLOCKED_FOR_CURSOR, false);
      }
    } else {
      int current_player_id = game_get_current_player(game)->id;
      for (int y = 0; y < game->board_height; y++) {
        for (int x = 0; x < game->board_width; x++) {
          Coords coords = { x, y };
          int tile = get_tile(game, coords);
          bool blocked = get_tile_player_id(tile) != current_player_id;
          this->set_tile_attr(coords, TILE_BLOCKED_FOR_CURSOR, blocked);
          this->set_tile_attr(coords, TILE_BLOCKED, false);
        }
      }
    }
  } else {
    this->set_all_tiles_attr(TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
  }

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      wxByte* attrs = this->tile_attrs_ptr(coords);
      bool was_blocked = (*attrs & TILE_BLOCKED_BEFORE) != 0;
      bool is_blocked = (*attrs & TILE_BLOCKED) != 0;
      if (is_blocked != was_blocked) {
        this->set_tile_attr(coords, TILE_BLOCKED_DIRTY, true);
      }
      this->set_tile_attr(coords, TILE_BLOCKED_BEFORE, is_blocked);
    }
  }
}

void CanvasPanel::on_paint(wxPaintEvent& WXUNUSED(event)) {
  wxPaintDC window_dc(this);

  wxSize size = this->get_canvas_size();
  if (!(size.x > 0 && size.y > 0)) {
    this->board_bitmap.UnRef();
    this->tiles_bitmap.UnRef();
    return;
  }

  wxRect update_region = GetUpdateRegion().GetBox();

  this->update_blocked_tiles();

  if (!this->tiles_bitmap.IsOk() || this->tiles_bitmap.GetSize() != size) {
    this->tiles_bitmap.Create(size, 24);
  }
  if (!this->board_bitmap.IsOk() || this->board_bitmap.GetSize() != size) {
    this->board_bitmap.Create(size, 24);
  }
  this->tiles_dc.SelectObject(this->tiles_bitmap);
  this->paint_tiles(this->tiles_dc, update_region);
  this->board_dc.SelectObject(this->board_bitmap);
  this->paint_board(this->board_dc, update_region, this->tiles_dc);

  wxPoint update_pos = update_region.GetPosition();
  window_dc.Blit(update_pos, update_region.GetSize(), &this->board_dc, update_pos);
  this->board_dc.SelectObject(wxNullBitmap);
  this->tiles_dc.SelectObject(wxNullBitmap);

  this->paint_overlay(window_dc);
}

void CanvasPanel::draw_bitmap(wxDC& dc, const wxBitmap& bitmap, const wxPoint& pos) {
#ifdef __WXMSW__
  // This works faster on Windows:
  wxMemoryDC& bmp_dc = this->draw_bitmap_dc;
  bmp_dc.SelectObjectAsSource(bitmap);
  dc.Blit(pos, bmp_dc.GetSize(), &bmp_dc, wxPoint(0, 0), wxCOPY);
#else
  dc.DrawBitmap(bitmap, pos);
#endif
}

void CanvasPanel::paint_tiles(wxDC& dc, const wxRect& update_region) {
  auto& tileset = wxGetApp().tileset;

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      if ((*this->tile_attrs_ptr(coords) & TILE_DIRTY) == 0) continue;
      wxRect tile_rect = this->get_tile_rect(coords);
      if (!update_region.Intersects(tile_rect)) continue;
      this->set_tile_attr(coords, TILE_DIRTY, false);
      // The next layer of the board has to be repainted though.
      this->set_tile_attr(coords, TILE_BLOCKED_DIRTY, true);

      int tile = get_tile(game, coords);
      wxPoint tile_pos = tile_rect.GetPosition();

      if (is_water_tile(tile)) {
        this->draw_bitmap(
          dc, tileset.water_tiles[(x ^ y) % WXSIZEOF(tileset.water_tiles)], tile_pos
        );
        continue;
      }

      this->draw_bitmap(dc, tileset.ice_tiles[(x ^ y) % WXSIZEOF(tileset.ice_tiles)], tile_pos);

      auto check_water = [&](int dx, int dy) -> bool {
        Coords neighbor = { coords.x + dx, coords.y + dy };
        return is_tile_in_bounds(game, neighbor) && is_water_tile(get_tile(game, neighbor));
      };

      auto draw_edge = [&](int dx, int dy, TileEdge type) {
        if (check_water(dx, dy)) {
          this->draw_bitmap(dc, tileset.tile_edges[type], tile_pos);
        }
      };
      draw_edge(0, -1, EDGE_TOP);
      draw_edge(1, 0, EDGE_RIGHT);
      draw_edge(0, 1, EDGE_BOTTOM);
      draw_edge(-1, 0, EDGE_LEFT);

      auto draw_concave_corner = [&](int dx, int dy, TileCorner type) -> void {
        if (check_water(dx, dy) && !check_water(dx, 0) && !check_water(0, dy)) {
          this->draw_bitmap(dc, tileset.tile_concave_corners[type], tile_pos);
        }
      };
      draw_concave_corner(1, -1, CORNER_TOP_RIGHT);
      draw_concave_corner(1, 1, CORNER_BOTTOM_RIGHT);
      draw_concave_corner(-1, 1, CORNER_BOTTOM_LEFT);
      draw_concave_corner(-1, -1, CORNER_TOP_LEFT);

      auto draw_convex_corner = [&](int dx, int dy, TileCorner type) -> void {
        if (check_water(dx, 0) && check_water(0, dy)) {
          this->draw_bitmap(dc, tileset.tile_convex_corners[type], tile_pos);
        }
      };
      draw_convex_corner(1, -1, CORNER_TOP_RIGHT);
      draw_convex_corner(1, 1, CORNER_BOTTOM_RIGHT);
      draw_convex_corner(-1, 1, CORNER_BOTTOM_LEFT);
      draw_convex_corner(-1, -1, CORNER_TOP_LEFT);

      if (is_fish_tile(tile)) {
        int fish = get_tile_fish(tile);
        this->draw_bitmap(
          dc, tileset.fish_sprites[(fish - 1) % WXSIZEOF(tileset.fish_sprites)], tile_pos
        );
      }
    }
  }
}

void CanvasPanel::paint_board(wxDC& dc, const wxRect& update_region, wxDC& tiles_dc) {
  auto& tileset = wxGetApp().tileset;

  Coords mouse_coords = this->tile_coords_at_point(this->mouse_pos);

  bool is_penguin_selected = false;
  Coords selected_penguin = { -1, -1 };
  if (game->phase == GAME_PHASE_MOVEMENT) {
    selected_penguin = this->get_selected_penguin_coords();
    is_penguin_selected = is_tile_in_bounds(game, selected_penguin);
  }

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      wxByte tile_attrs = *this->tile_attrs_ptr(coords);
      if ((tile_attrs & TILE_BLOCKED_DIRTY) == 0) continue;
      wxRect tile_rect = this->get_tile_rect(coords);
      if (!update_region.Intersects(tile_rect)) continue;
      this->set_tile_attr(coords, TILE_BLOCKED_DIRTY, false);

      int tile = get_tile(game, coords);
      wxPoint tile_pos = tile_rect.GetPosition();
      dc.Blit(tile_pos, tile_rect.GetSize(), &tiles_dc, tile_pos);

      if ((tile_attrs & TILE_BLOCKED) != 0) {
        this->draw_bitmap(dc, tileset.blocked_tile, tile_pos);
      }

      if (is_penguin_tile(tile)) {
        int player = game_find_player_by_id(game, get_tile_player_id(tile));
        assert(player >= 0);
        bool flipped = false;
        if (is_penguin_selected && coords_same(coords, selected_penguin)) {
          flipped = mouse_coords.x < selected_penguin.x;
        }
        wxBitmap* penguin_sprites =
          flipped ? tileset.penguin_sprites_flipped : tileset.penguin_sprites;
        this->draw_bitmap(
          dc, penguin_sprites[player % WXSIZEOF(tileset.penguin_sprites)], tile_pos
        );
      }

      this->draw_bitmap(dc, tileset.grid_tile, tile_pos);
    }
  }
}

enum ArrowHeadType {
  ARROW_HEAD_NORMAL = 1,
  ARROW_HEAD_CROSS = 2,
};

static void draw_arrow_head(
  wxDC& dc,
  wxPoint start,
  wxPoint end,
  wxSize head_size,
  ArrowHeadType head_type = ARROW_HEAD_NORMAL
) {
  if (start == end) return;
  wxPoint2DDouble norm(end - start);
  norm.Normalize();
  wxPoint2DDouble perp(-norm.m_y, norm.m_x);
  wxPoint2DDouble head1 = -norm * head_size.x + perp * head_size.y;
  wxPoint2DDouble head2 = -norm * head_size.x - perp * head_size.y;
  wxPoint head1i(head1.m_x, head1.m_y), head2i(head2.m_x, head2.m_y);
  if (head_type == ARROW_HEAD_NORMAL) {
    dc.DrawLine(end, end + head1i);
    dc.DrawLine(end, end + head2i);
  } else if (head_type == ARROW_HEAD_CROSS) {
    dc.DrawLine(end - head1i, end + head1i);
    dc.DrawLine(end - head2i, end + head2i);
  }
}

void CanvasPanel::paint_overlay(wxDC& dc) {
  if (this->game_frame->get_current_player_type() != PLAYER_NORMAL) return;

  if (this->mouse_within_window && game->phase != GAME_PHASE_END) {
    Coords current_coords = this->tile_coords_at_point(this->mouse_pos);
    if (is_tile_in_bounds(game, current_coords)) {
      wxByte tile_attrs = *this->tile_attrs_ptr(current_coords);
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen((tile_attrs & TILE_BLOCKED_FOR_CURSOR) != 0 ? *wxRED : *wxGREEN, 5));
      wxRect rect = this->get_tile_rect(current_coords);
      dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
  }

  if (this->mouse_is_down && game->phase == GAME_PHASE_MOVEMENT) {
    Coords penguin = this->get_selected_penguin_coords();
    Coords target = this->tile_coords_at_point(this->mouse_pos);
    if (is_tile_in_bounds(game, penguin) && is_tile_in_bounds(game, target)) {
      if (!coords_same(penguin, target)) {
        Coords move_fail = penguin;
        MovementError result = validate_movement(game, penguin, target, &move_fail);
        wxPoint arrow_start = this->get_tile_centre(penguin);
        wxPoint arrow_fail = this->get_tile_centre(move_fail);
        wxPoint arrow_end = this->get_tile_centre(target);

        wxSize head_size(8, 8);
        wxPen bg_pen(*wxBLACK, 6);
        wxPen green_pen((*wxGREEN).ChangeLightness(75), 4);
        wxPen red_pen((*wxRED).ChangeLightness(75), 4);

        if (result != VALID_INPUT && !coords_same(move_fail, penguin)) {
          dc.SetPen(bg_pen);
          dc.DrawLine(arrow_start, arrow_fail);
          dc.SetPen(green_pen);
          dc.DrawLine(arrow_start, arrow_fail);
          dc.SetPen(bg_pen);
          draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
          dc.DrawLine(arrow_fail, arrow_end);
          draw_arrow_head(dc, arrow_fail, arrow_end, head_size);
          dc.SetPen(red_pen);
          draw_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
          dc.DrawLine(arrow_fail, arrow_end);
          draw_arrow_head(dc, arrow_fail, arrow_end, head_size);
        } else {
          dc.SetPen(bg_pen);
          dc.DrawLine(arrow_start, arrow_end);
          draw_arrow_head(dc, arrow_start, arrow_end, head_size);
          dc.SetPen(result == VALID_INPUT ? green_pen : red_pen);
          dc.DrawLine(arrow_start, arrow_end);
          draw_arrow_head(dc, arrow_start, arrow_end, head_size);
        }
      }
    }
  }
}

void CanvasPanel::on_any_mouse_event(wxMouseEvent& event) {
  this->prev_mouse_pos = this->mouse_pos;
  this->mouse_pos = event.GetPosition();

  if (!this->mouse_is_down) {
    this->mouse_drag_pos = this->mouse_pos;
  }
  if (event.ButtonDown()) {
    this->mouse_is_down = true;
    this->mouse_drag_pos = this->mouse_pos;
  } else if (event.ButtonUp()) {
    this->mouse_is_down = false;
  }

  if (event.Entering()) {
    this->mouse_within_window = true;
  } else if (event.Leaving()) {
    this->mouse_within_window = false;
  }

  // NOTE: a `switch` is unusable here because the event types are defined as
  // `extern` variables. `switch` in C++ can only work with statically-known
  // constants.
  auto event_type = event.GetEventType();
  if (event_type == wxEVT_LEFT_DOWN) {
    this->on_mouse_down(event);
  } else if (event_type == wxEVT_MOTION) {
    this->on_mouse_move(event);
  } else if (event_type == wxEVT_LEFT_UP) {
    this->on_mouse_up(event);
  } else if (event_type == wxEVT_ENTER_WINDOW) {
    this->on_mouse_enter_leave(event);
  } else if (event_type == wxEVT_LEAVE_WINDOW) {
    this->on_mouse_enter_leave(event);
  } else {
    event.Skip();
  }
}

void CanvasPanel::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {
  if (this->game_frame->get_current_player_type() == PLAYER_NORMAL) {
    if (game->phase == GAME_PHASE_MOVEMENT) {
      this->Refresh();
    }
  }
}

void CanvasPanel::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->tile_coords_at_point(this->prev_mouse_pos);
  Coords curr_coords = this->tile_coords_at_point(this->mouse_pos);
  if (!coords_same(curr_coords, prev_coords)) {
    if (this->game_frame->get_current_player_type() == PLAYER_NORMAL) {
      Coords selected_penguin = this->get_selected_penguin_coords();
      if (is_tile_in_bounds(game, selected_penguin) && game->phase == GAME_PHASE_MOVEMENT) {
        this->set_tile_attr(selected_penguin, TILE_DIRTY, true);
      }
    }
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  if (this->game_frame->get_current_player_type() == PLAYER_BOT) {
    if (!this->game_frame->executing_bot_turn) {
      this->game_frame->execute_bot_turn();
    }
    return;
  }
  Coords prev_coords = this->tile_coords_at_point(this->mouse_drag_pos);
  Coords curr_coords = this->tile_coords_at_point(this->mouse_pos);
  if (game->phase == GAME_PHASE_PLACEMENT) {
    if (is_tile_in_bounds(game, curr_coords) && coords_same(prev_coords, curr_coords)) {
      if (validate_placement(game, curr_coords) == PLACEMENT_VALID) {
        this->game_frame->place_penguin(curr_coords);
        this->game_frame->update_game_state();
        this->Refresh();
      }
    }
  } else if (game->phase == GAME_PHASE_MOVEMENT) {
    if (is_tile_in_bounds(game, curr_coords) && is_tile_in_bounds(game, prev_coords)) {
      if (validate_movement(game, prev_coords, curr_coords, nullptr) == VALID_INPUT) {
        this->game_frame->move_penguin(prev_coords, curr_coords);
        this->game_frame->update_game_state();
      }
    }
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_enter_leave(wxMouseEvent& WXUNUSED(event)) {
  this->Refresh();
}
