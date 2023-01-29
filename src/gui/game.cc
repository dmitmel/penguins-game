#include "gui/game.hh"
#include "board.h"
#include "bot.h"
#include "game.h"
#include "gui/game_end_dialog.hh"
#include "gui/game_state.hh"
#include "gui/new_game_dialog.hh"
#include "gui/player_info_box.hh"
#include "movement.h"
#include "placement.h"
#include "resources_appicon_256_png.h"
#include "utils.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>
#include <wx/bitmap.h>
#include <wx/brush.h>
#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/graphics.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/pen.h>
#include <wx/peninfobase.h>
#include <wx/persist.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/types.h>
#include <wx/window.h>

static inline bool coords_same(const Coords& a, const Coords& b) {
  return a.x == b.x && a.y == b.y;
}

enum {
  ID_NEW_GAME = wxID_HIGHEST + 1,
  ID_CLOSE_GAME,
};

// clang-format off
wxBEGIN_EVENT_TABLE(GameFrame, wxFrame)
  EVT_MENU(ID_NEW_GAME, GameFrame::on_new_game)
  EVT_MENU(ID_CLOSE_GAME, GameFrame::on_close_game)
  EVT_MENU(wxID_ABOUT, GameFrame::on_about)
  EVT_MENU(wxID_EXIT, GameFrame::on_exit)
wxEND_EVENT_TABLE();
// clang-format on

GameFrame::GameFrame(
  wxWindow* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset
)
: wxFrame(parent, id, "Penguins game"), state(state), tileset(tileset) {
  wxMemoryInputStream icon_stream(resources_appicon_256_png, resources_appicon_256_png_size);
  wxIcon app_icon;
  // The triple conversion necessary to load the icon here is... meh.
  app_icon.CopyFromBitmap(wxBitmap(wxImage(icon_stream, wxBITMAP_TYPE_PNG)));
  this->SetIcon(app_icon);

#ifdef __WXMSW__
  this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
#endif

  auto menu_file = new wxMenu();
  menu_file->Append(ID_NEW_GAME, "&New game\tCtrl-N", "Start a new game");
  menu_file->Append(ID_CLOSE_GAME, "&Close the game\tCtrl-W", "Close the current game");
  menu_file->AppendSeparator();
  menu_file->Append(wxID_EXIT);

  auto menu_help = new wxMenu();
  menu_help->Append(wxID_ABOUT);

  auto menu_bar = new wxMenuBar();
  menu_bar->Append(menu_file, "&File");
  menu_bar->Append(menu_help, "&Help");

  this->SetMenuBar(menu_bar);

  auto root_vbox = new wxBoxSizer(wxVERTICAL);

  this->canvas_panel = new CanvasPanel(this, wxID_ANY, state, tileset);

  this->players_box = new wxBoxSizer(wxHORIZONTAL);
  root_vbox->Add(players_box, wxSizerFlags().Centre().Border(wxALL & ~wxDOWN));

  auto canvas_hbox = new wxBoxSizer(wxHORIZONTAL);
  canvas_hbox->Add(this->canvas_panel, wxSizerFlags(1).Centre().Border());
  root_vbox->Add(canvas_hbox, wxSizerFlags(1).Centre().Border());

  this->SetSizerAndFit(root_vbox);

  this->CreateStatusBar();
  this->SetStatusText("Welcome to wxWidgets!");
}

void GameFrame::on_new_game(wxCommandEvent& WXUNUSED(event)) {
  this->start_new_game();
}

void GameFrame::on_close_game(wxCommandEvent& WXUNUSED(event)) {
  this->close_game();
}

void GameFrame::on_exit(wxCommandEvent& WXUNUSED(event)) {
  this->Close(true);
}

void GameFrame::on_about(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION
  );
}

void GameFrame::start_new_game() {
  std::unique_ptr<NewGameDialog> dialog(new NewGameDialog(this, wxID_ANY));
  wxPersistentRegisterAndRestore(dialog.get());
  int result = dialog->ShowModal();
  if (result != wxID_OK) {
    return;
  }

  state.game_ended = false;
  Game* game = game_new();
  state.game.reset(game);
  state.bot_state.reset(nullptr);
  state.bot_params.reset(nullptr);
  state.player_names.reset(new wxString[dialog->get_number_of_players()]);
  state.player_types.reset(new PlayerType[dialog->get_number_of_players()]);

  game_begin_setup(game);
  game_set_penguins_per_player(game, dialog->get_penguins_per_player());
  game_set_players_count(game, dialog->get_number_of_players());
  bool any_bots = false;
  for (int i = 0; i < game->players_count; i++) {
    game_set_player_name(game, i, dialog->get_player_name(i).c_str());
    state.player_names[i] = dialog->get_player_name(i);
    state.player_types[i] = dialog->get_player_type(i);
    any_bots = any_bots || dialog->get_player_type(i) != PLAYER_NORMAL;
  }
  setup_board(game, dialog->get_board_width(), dialog->get_board_height());
  switch (dialog->get_board_gen_type()) {
    case BOARD_GEN_RANDOM: generate_board_random(game); break;
    case BOARD_GEN_ISLAND: generate_board_island(game); break;
    default: break;
  }
  game_end_setup(game);

  if (any_bots) {
    state.bot_params.reset(new BotParameters);
    init_bot_parameters(state.bot_params.get());
    state.bot_state.reset(bot_state_new(state.bot_params.get(), game));
  }

  this->canvas_panel->cell_attributes.reset(new wxByte[game->board_width * game->board_height]);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords cell = { x, y };
      *this->canvas_panel->cell_attrs_ptr(cell) = CELL_DIRTY | CELL_BLOCKED_DIRTY;
    }
  }

  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes.reset(new PlayerInfoBox*[game->players_count]);

  for (int i = 0; i < game->players_count; i++) {
    auto player_box = new PlayerInfoBox(
      this, wxID_ANY, game_get_player(game, i)->id, dialog->get_player_name(i), this->tileset
    );
    this->players_box->Add(
      player_box->GetContainingSizer(), wxSizerFlags().Border(wxALL & ~wxDOWN)
    );
    this->player_info_boxes[i] = player_box;
  }
  this->update_player_info_boxes();

  this->canvas_panel->InvalidateBestSize();
  this->GetSizer()->SetSizeHints(this);
  this->Refresh();
  this->Centre();

  this->update_game_state();
  this->canvas_panel->update_blocked_cells();
}

void GameFrame::update_game_state() {
  Game* game = this->state.game.get();
  while (true) {
    game_advance_state(game);
    this->update_player_info_boxes();
    if (game->phase == GAME_PHASE_END) {
      if (!this->state.game_ended) {
        this->state.game_ended = true;
        this->end_game();
      }
      break;
    }
    int current_player_idx = game->current_player_index;
    if (0 <= current_player_idx && current_player_idx < game->players_count) {
      if (state.player_types[current_player_idx] != PLAYER_NORMAL) {
        if (game->phase == GAME_PHASE_PLACEMENT) {
          Coords target;
          if (bot_make_placement(state.bot_state.get(), &target)) {
            this->place_penguin(target);
          }
          continue;
        } else if (game->phase == GAME_PHASE_MOVEMENT) {
          Coords penguin, target;
          if (bot_make_move(state.bot_state.get(), &penguin, &target)) {
            this->move_penguin(penguin, target);
          }
          continue;
        }
      }
    }
    break;
  }
}

void GameFrame::place_penguin(Coords target) {
  ::place_penguin(this->state.game.get(), target);
  this->canvas_panel->set_cell_attr(target, CELL_DIRTY, true);
  this->canvas_panel->set_cell_neighbors_attr(target, CELL_DIRTY, true);
}

void GameFrame::move_penguin(Coords penguin, Coords target) {
  ::move_penguin(this->state.game.get(), penguin, target);
  this->canvas_panel->set_cell_attr(penguin, CELL_DIRTY, true);
  this->canvas_panel->set_cell_neighbors_attr(penguin, CELL_DIRTY, true);
  this->canvas_panel->set_cell_attr(target, CELL_DIRTY, true);
  this->canvas_panel->set_cell_neighbors_attr(target, CELL_DIRTY, true);
}

void GameFrame::end_game() {
  std::unique_ptr<GameEndDialog> dialog(new GameEndDialog(this, wxID_ANY, this->state));
  dialog->ShowModal();
}

void GameFrame::close_game() {
  this->state.game.reset(nullptr);
  this->state.bot_params.reset(nullptr);
  this->state.bot_state.reset(nullptr);
  this->state.player_names.reset(nullptr);
  this->state.player_types.reset(nullptr);
  this->canvas_panel->cell_attributes.reset(nullptr);
  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes.reset(nullptr);

  this->canvas_panel->InvalidateBestSize();
  this->GetSizer()->SetSizeHints(this);
  this->Refresh();
  this->Centre();
}

void GameFrame::update_player_info_boxes() {
  Game* game = this->state.game.get();
  if (!game) return;
  for (int i = 0; i < game->players_count; i++) {
    PlayerInfoBox* player_box = this->player_info_boxes[i];
    Player* player = game_get_player(game, i);
    player_box->is_current = i == game->current_player_index;
    if (game->phase == GAME_PHASE_MOVEMENT) {
      player_box->is_blocked = true;
      for (int j = 0; j < player->penguins_count; j++) {
        PossibleSteps moves = calculate_penguin_possible_moves(game, player->penguins[j]);
        int steps_sum = 0;
        for (int dir = 0; dir < DIRECTION_MAX; dir++) {
          steps_sum += moves.steps[dir];
        }
        if (steps_sum != 0) {
          player_box->is_blocked = false;
          break;
        }
      }
    } else {
      player_box->is_blocked = false;
    }
    player_box->set_score(player->points);
    player_box->penguin_sprite = this->canvas_panel->get_player_penguin_sprite(player->id);
    player_box->Refresh();
  }
}

// clang-format off
wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
  EVT_PAINT(CanvasPanel::on_paint)
  EVT_MOUSE_EVENTS(CanvasPanel::on_any_mouse_event)
wxEND_EVENT_TABLE();
// clang-format on

CanvasPanel::CanvasPanel(
  GameFrame* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset
)
: wxPanel(parent, id), game_frame(parent), state(state), tileset(tileset) {
#ifdef __WXMSW__
  // Necessary to avoid flicker on Windows, see <https://wiki.wxwidgets.org/Flicker-Free_Drawing>.
  this->SetDoubleBuffered(true);
#endif
}

wxSize CanvasPanel::get_canvas_size() const {
  if (Game* game = this->state.game.get()) {
    return CELL_SIZE * wxSize(game->board_width, game->board_height);
  } else {
    return wxSize(0, 0);
  }
}

Coords CanvasPanel::get_cell_by_coords(wxPoint point) const {
  return { point.x / CELL_SIZE, point.y / CELL_SIZE };
}

wxRect CanvasPanel::get_cell_rect(Coords cell) const {
  return wxRect(cell.x * CELL_SIZE, cell.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
}

wxPoint CanvasPanel::get_cell_centre(Coords cell) const {
  wxRect rect = this->get_cell_rect(cell);
  return rect.GetPosition() + rect.GetSize() / 2;
}

Coords CanvasPanel::get_selected_penguin_cell(int player_index) const {
  Coords null_coords = { -1, -1 };
  Game* game = this->state.game.get();
  if (!game) return null_coords;
  if (!(0 <= player_index && player_index < game->players_count)) return null_coords;
  int player_id = game_get_player(game, player_index)->id;
  wxPoint curr_cell_pos = this->mouse_is_down ? this->mouse_drag_pos : this->mouse_pos;
  Coords curr_cell = this->get_cell_by_coords(curr_cell_pos);
  if (!is_tile_in_bounds(game, curr_cell)) return null_coords;
  int tile = get_tile(game, curr_cell);
  return get_tile_player_id(tile) == player_id ? curr_cell : null_coords;
}

wxByte* CanvasPanel::cell_attrs_ptr(Coords cell) const {
  if (Game* game = this->state.game.get()) {
    assert(is_tile_in_bounds(game, cell));
    return &this->cell_attributes[cell.x + cell.y * game->board_width];
  } else {
    return nullptr;
  }
}

void CanvasPanel::set_cell_attr(Coords cell, wxByte attr, bool value) {
  wxByte* cell_attrs = this->cell_attrs_ptr(cell);
  *cell_attrs = (*cell_attrs & ~attr) | (value ? attr : 0);
}

void CanvasPanel::set_cell_neighbors_attr(Coords cell, wxByte attr, bool value) {
  for (int dir = 0; dir < NEIGHBOR_MAX; dir++) {
    Coords neighbor = NEIGHBOR_TO_COORDS[dir];
    neighbor.x += cell.x, neighbor.y += cell.y;
    if (!is_tile_in_bounds(this->state.game.get(), neighbor)) continue;
    this->set_cell_attr(neighbor, attr, value);
  }
}

void CanvasPanel::set_all_cells_attr(wxByte attr, bool value) {
  if (Game* game = this->state.game.get()) {
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords cell = { x, y };
        this->set_cell_attr(cell, attr, value);
      }
    }
  }
}

void CanvasPanel::update_blocked_cells() {
  Game* game = this->state.game.get();
  if (!game) return;

  if (game->phase == GAME_PHASE_PLACEMENT) {
    Coords curr_cell = this->get_cell_by_coords(this->mouse_pos);
    bool is_a_cell_selected = this->mouse_within_window && is_tile_in_bounds(game, curr_cell);
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords cell = { x, y };
        bool blocked = !validate_placement_simple(game, cell);
        this->set_cell_attr(cell, CELL_BLOCKED_FOR_CURSOR, blocked);
        this->set_cell_attr(cell, CELL_BLOCKED, blocked && is_a_cell_selected);
      }
    }
  } else if (game->phase == GAME_PHASE_MOVEMENT) {
    Coords curr_cell = this->get_selected_penguin_cell(game->current_player_index);
    if (this->mouse_within_window && is_tile_in_bounds(game, curr_cell)) {
      // A penguin is selected
      for (int y = 0; y < game->board_height; y++) {
        for (int x = 0; x < game->board_width; x++) {
          Coords cell = { x, y };
          this->set_cell_attr(cell, CELL_BLOCKED | CELL_BLOCKED_FOR_CURSOR, true);
        }
      }
      PossibleSteps moves = calculate_penguin_possible_moves(game, curr_cell);
      int steps_sum = 0;
      for (int dir = 0; dir < DIRECTION_MAX; dir++) {
        Coords cell = curr_cell;
        Coords d = DIRECTION_TO_COORDS[dir];
        steps_sum += moves.steps[dir];
        for (int steps = moves.steps[dir]; steps > 0; steps--) {
          cell.x += d.x, cell.y += d.y;
          this->set_cell_attr(cell, CELL_BLOCKED | CELL_BLOCKED_FOR_CURSOR, false);
        }
      }
      this->set_cell_attr(curr_cell, CELL_BLOCKED, false);
      if (steps_sum != 0 && !this->mouse_is_down) {
        this->set_cell_attr(curr_cell, CELL_BLOCKED_FOR_CURSOR, false);
      }
    } else {
      int current_player_id = game_get_current_player(game)->id;
      for (int y = 0; y < game->board_height; y++) {
        for (int x = 0; x < game->board_width; x++) {
          Coords cell = { x, y };
          int tile = get_tile(game, cell);
          bool blocked = get_tile_player_id(tile) != current_player_id;
          this->set_cell_attr(cell, CELL_BLOCKED_FOR_CURSOR, blocked);
          this->set_cell_attr(cell, CELL_BLOCKED, false);
        }
      }
    }
  } else {
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords cell = { x, y };
        this->set_cell_attr(cell, CELL_BLOCKED | CELL_BLOCKED_FOR_CURSOR, false);
      }
    }
  }

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords cell = { x, y };
      wxByte* attrs = this->cell_attrs_ptr(cell);
      bool was_blocked = (*attrs & CELL_BLOCKED_BEFORE) != 0;
      bool is_blocked = (*attrs & CELL_BLOCKED) != 0;
      if (is_blocked != was_blocked) {
        this->set_cell_attr(cell, CELL_BLOCKED_DIRTY, true);
      }
      this->set_cell_attr(cell, CELL_BLOCKED_BEFORE, is_blocked);
    }
  }
}

wxSize CanvasPanel::DoGetBestClientSize() const {
  if (!this->state.game.get()) {
    wxSize default_size(NewGameDialog::DEFAULT_BOARD_WIDTH, NewGameDialog::DEFAULT_BOARD_HEIGHT);
    return CELL_SIZE * default_size;
  }
  return this->get_canvas_size();
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
  this->set_all_cells_attr(CELL_DIRTY | CELL_BLOCKED_DIRTY, false);

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
  Game* game = this->state.game.get();
  if (!game) return;

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords cell = { x, y };
      if ((*this->cell_attrs_ptr(cell) & CELL_DIRTY) == 0) continue;
      wxRect cell_rect = this->get_cell_rect(cell);
      if (!update_region.Intersects(cell_rect)) continue;
      int cell_value = get_tile(game, cell);
      wxPoint cell_pos = cell_rect.GetPosition();

      if (is_water_tile(cell_value)) {
        this->draw_bitmap(
          dc, tileset.water_tiles[(x ^ y) % WXSIZEOF(tileset.water_tiles)], cell_pos
        );
        continue;
      }

      this->draw_bitmap(dc, tileset.ice_tiles[(x ^ y) % WXSIZEOF(tileset.ice_tiles)], cell_pos);

      auto check_water = [&](int dx, int dy) -> bool {
        Coords cell2 = { cell.x + dx, cell.y + dy };
        return is_tile_in_bounds(game, cell2) && is_water_tile(get_tile(game, cell2));
      };

      auto draw_edge = [&](int dx, int dy, TileEdge type) {
        if (check_water(dx, dy)) {
          this->draw_bitmap(dc, tileset.tile_edges[type], cell_pos);
        }
      };
      draw_edge(0, -1, EDGE_TOP);
      draw_edge(1, 0, EDGE_RIGHT);
      draw_edge(0, 1, EDGE_BOTTOM);
      draw_edge(-1, 0, EDGE_LEFT);

      auto draw_concave_corner = [&](int dx, int dy, TileCorner type) -> void {
        if (check_water(dx, dy) && !check_water(dx, 0) && !check_water(0, dy)) {
          this->draw_bitmap(dc, tileset.tile_concave_corners[type], cell_pos);
        }
      };
      draw_concave_corner(1, -1, CORNER_TOP_RIGHT);
      draw_concave_corner(1, 1, CORNER_BOTTOM_RIGHT);
      draw_concave_corner(-1, 1, CORNER_BOTTOM_LEFT);
      draw_concave_corner(-1, -1, CORNER_TOP_LEFT);

      auto draw_convex_corner = [&](int dx, int dy, TileCorner type) -> void {
        if (check_water(dx, 0) && check_water(0, dy)) {
          this->draw_bitmap(dc, tileset.tile_convex_corners[type], cell_pos);
        }
      };
      draw_convex_corner(1, -1, CORNER_TOP_RIGHT);
      draw_convex_corner(1, 1, CORNER_BOTTOM_RIGHT);
      draw_convex_corner(-1, 1, CORNER_BOTTOM_LEFT);
      draw_convex_corner(-1, -1, CORNER_TOP_LEFT);

      if (is_fish_tile(cell_value)) {
        int fish_count = get_tile_fish(cell_value);
        this->draw_bitmap(
          dc, tileset.fish_sprites[(fish_count - 1) % WXSIZEOF(tileset.fish_sprites)], cell_pos
        );
      }
    }
  }
}

void CanvasPanel::paint_board(wxDC& dc, const wxRect& update_region, wxDC& tiles_dc) {
  Game* game = this->state.game.get();
  if (!game) return;

  Coords mouse_cell = this->get_cell_by_coords(this->mouse_pos);

  bool is_penguin_selected = false;
  Coords selected_penguin_cell = { -1, -1 };
  if (game->phase == GAME_PHASE_MOVEMENT) {
    selected_penguin_cell = this->get_selected_penguin_cell(game->current_player_index);
    is_penguin_selected = is_tile_in_bounds(game, selected_penguin_cell);
  }

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords cell = { x, y };
      wxByte cell_attrs = *this->cell_attrs_ptr(cell);
      if ((cell_attrs & (CELL_DIRTY | CELL_BLOCKED_DIRTY)) == 0) continue;
      wxRect cell_rect = this->get_cell_rect(cell);
      if (!update_region.Intersects(cell_rect)) continue;
      int cell_value = get_tile(game, cell);
      wxPoint cell_pos = cell_rect.GetPosition();

      dc.Blit(cell_pos, cell_rect.GetSize(), &tiles_dc, cell_pos);

      if ((cell_attrs & CELL_BLOCKED) != 0) {
        this->draw_bitmap(dc, tileset.blocked_tile, cell_pos);
      }

      if (is_penguin_tile(cell_value)) {
        int player = get_tile_player_id(cell_value);
        bool flipped = false;
        if (is_penguin_selected && coords_same(cell, selected_penguin_cell)) {
          flipped = mouse_cell.x < selected_penguin_cell.x;
        }
        this->draw_bitmap(dc, this->get_player_penguin_sprite(player, flipped), cell_pos);
      }

      this->draw_bitmap(dc, tileset.grid_tile, cell_pos);
    }
  }
}

const wxBitmap& CanvasPanel::get_player_penguin_sprite(int player_id, bool flipped) const {
  size_t idx = size_t(player_id - 1) % WXSIZEOF(tileset.penguin_sprites);
  return flipped ? tileset.penguin_sprites_flipped[idx] : tileset.penguin_sprites[idx];
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
  Game* game = this->state.game.get();
  if (!game) return;

  if (this->mouse_within_window && game->phase != GAME_PHASE_END) {
    Coords current_cell = this->get_cell_by_coords(this->mouse_pos);
    if (is_tile_in_bounds(game, current_cell)) {
      wxByte cell_attrs = *this->cell_attrs_ptr(current_cell);
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen((cell_attrs & CELL_BLOCKED_FOR_CURSOR) != 0 ? *wxRED : *wxGREEN, 5));
      wxRect rect = this->get_cell_rect(current_cell);
      dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
  }

  if (this->mouse_is_down && game->phase == GAME_PHASE_MOVEMENT) {
    Coords start_cell = this->get_selected_penguin_cell(game->current_player_index);
    Coords dest_cell = this->get_cell_by_coords(this->mouse_pos);
    if (is_tile_in_bounds(game, start_cell) && is_tile_in_bounds(game, dest_cell)) {
      if (!coords_same(start_cell, dest_cell)) {
        Coords move_fail_cell(start_cell);
        MovementError result = validate_movement(game, start_cell, dest_cell, &move_fail_cell);
        wxPoint arrow_start = this->get_cell_centre(start_cell);
        wxPoint arrow_fail = this->get_cell_centre(move_fail_cell);
        wxPoint arrow_end = this->get_cell_centre(dest_cell);

        wxSize head_size(8, 8);
        wxPen bg_pen(*wxBLACK, 6);
        wxPen green_pen((*wxGREEN).ChangeLightness(75), 4);
        wxPen red_pen((*wxRED).ChangeLightness(75), 4);

        if (result != VALID_INPUT && !coords_same(move_fail_cell, start_cell)) {
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

  if (!this->state.game.get()) {
    event.Skip();
    return;
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
  Game* game = this->state.game.get();
  if (game->phase == GAME_PHASE_MOVEMENT) {
    this->update_blocked_cells();
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Game* game = this->state.game.get();
  Coords prev_cell = this->get_cell_by_coords(this->prev_mouse_pos);
  Coords curr_cell = this->get_cell_by_coords(this->mouse_pos);
  if (!coords_same(curr_cell, prev_cell)) {
    if (is_tile_in_bounds(game, curr_cell)) {
      this->update_blocked_cells();
    }
    Coords selected_penguin_cell = this->get_selected_penguin_cell(game->current_player_index);
    if (is_tile_in_bounds(game, selected_penguin_cell) && game->phase == GAME_PHASE_MOVEMENT) {
      this->set_cell_attr(selected_penguin_cell, CELL_DIRTY, true);
    }
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Game* game = this->state.game.get();
  Coords prev_cell = this->get_cell_by_coords(this->mouse_drag_pos);
  Coords curr_cell = this->get_cell_by_coords(this->mouse_pos);
  if (game->phase == GAME_PHASE_PLACEMENT) {
    if (is_tile_in_bounds(game, curr_cell) && coords_same(prev_cell, curr_cell)) {
      if (validate_placement(game, curr_cell) == PLACEMENT_VALID) {
        this->game_frame->place_penguin(curr_cell);
        this->game_frame->update_game_state();
        this->update_blocked_cells();
        this->Refresh();
      }
    }
  } else if (game->phase == GAME_PHASE_MOVEMENT) {
    if (is_tile_in_bounds(game, curr_cell) && is_tile_in_bounds(game, prev_cell)) {
      if (validate_movement(game, prev_cell, curr_cell, nullptr) == VALID_INPUT) {
        this->game_frame->move_penguin(prev_cell, curr_cell);
        this->game_frame->update_game_state();
      }
    }
    this->update_blocked_cells();
    this->Refresh();
  }
}

void CanvasPanel::on_mouse_enter_leave(wxMouseEvent& WXUNUSED(event)) {
  this->update_blocked_cells();
  this->Refresh();
}
