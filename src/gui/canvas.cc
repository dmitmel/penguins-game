#include "gui/canvas.hh"
#include "board.h"
#include "gui/controllers.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "gui/main.hh"
#include "gui/tileset.hh"
#include <cassert>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/region.h>
#include <wx/types.h>
#include <wx/window.h>

// clang-format off
wxBEGIN_EVENT_TABLE(CanvasPanel, wxPanel)
  EVT_PAINT(CanvasPanel::on_paint)
  EVT_MOUSE_EVENTS(CanvasPanel::on_any_mouse_event)
wxEND_EVENT_TABLE();
// clang-format on

CanvasPanel::CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame)
: wxPanel(parent, id)
, game_frame(game_frame)
, game(game_frame->state.game.get())
, tile_attributes(new wxByte[game->board_width * game->board_height]) {
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      *this->tile_attrs_ptr(coords) = TILE_DIRTY | TILE_BLOCKED_DIRTY;
    }
  }
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
  if (!this->mouse_within_window) return null_coords;
  wxPoint curr_tile_pos = this->mouse_is_down ? this->mouse_drag_pos : this->mouse_pos;
  Coords curr_coords = this->tile_coords_at_point(curr_tile_pos);
  if (!is_tile_in_bounds(game, curr_coords)) return null_coords;
  int player_idx = game->current_player_index;
  if (!game_check_player_index(game, player_idx)) return null_coords;
  int player_id = game_get_player(game, player_idx)->id;
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

void CanvasPanel::on_paint(wxPaintEvent& WXUNUSED(event)) {
  wxPaintDC window_dc(this);

  wxSize size = this->get_canvas_size();
  if (!(size.x > 0 && size.y > 0)) {
    this->board_bitmap.UnRef();
    this->tiles_bitmap.UnRef();
    return;
  }

  wxRect update_region = GetUpdateRegion().GetBox();

  this->game_frame->controller->update_tile_attributes();
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      wxByte attrs = *this->tile_attrs_ptr(coords);
      bool was_blocked = (attrs & TILE_BLOCKED_BEFORE) != 0;
      bool is_blocked = (attrs & TILE_BLOCKED) != 0;
      if (is_blocked != was_blocked) {
        this->set_tile_attr(coords, TILE_BLOCKED_DIRTY, true);
      }
      this->set_tile_attr(coords, TILE_BLOCKED_BEFORE, is_blocked);
    }
  }

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

  this->game_frame->controller->paint_overlay(window_dc);
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

void CanvasPanel::on_any_mouse_event(wxMouseEvent& event) {
  this->prev_mouse_pos = this->mouse_pos;
  this->mouse_pos = event.GetPosition();

  if (!this->mouse_is_down) {
    this->mouse_drag_pos = this->mouse_pos;
  }
  if (event.ButtonDown()) {
    this->mouse_is_down = true;
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
    this->game_frame->controller->on_mouse_down(event);
  } else if (event_type == wxEVT_MOTION) {
    this->game_frame->controller->on_mouse_move(event);
  } else if (event_type == wxEVT_LEFT_UP) {
    this->game_frame->controller->on_mouse_up(event);
  } else if (event_type == wxEVT_ENTER_WINDOW) {
    this->Refresh();
  } else if (event_type == wxEVT_LEAVE_WINDOW) {
    this->Refresh();
  } else {
    event.Skip();
  }
}
