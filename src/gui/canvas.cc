#include "gui/canvas.hh"
#include "board.h"
#include "game.h"
#include "gui/controllers.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "gui/main.hh"
#include "gui/tileset.hh"
#include "utils.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <wx/bitmap.h>
#include <wx/colour.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/geometry.h>
#include <wx/pen.h>
#include <wx/region.h>
#include <wx/window.h>

// clang-format off
wxBEGIN_EVENT_TABLE(CanvasPanel, wxWindow)
  EVT_PAINT(CanvasPanel::on_paint)
  EVT_MOUSE_EVENTS(CanvasPanel::on_any_mouse_event)
wxEND_EVENT_TABLE();
// clang-format on

CanvasPanel::CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame)
: wxWindow(parent, id), game_frame(game_frame), game(game_frame->state.game.get()) {
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

      if (get_tile_attr(game, coords, TILE_DIRTY)) {
        set_tile_attr(game, coords, TILE_DIRTY, false);
        set_tile_attr(game, coords, TILE_NEEDS_REDRAW, true);
        // Request repainting of the neighboring tiles too.
        for (int dir = 0; dir < NEIGHBOR_MAX; dir++) {
          Coords neighbor = NEIGHBOR_TO_COORDS[dir];
          neighbor.x += coords.x, neighbor.y += coords.y;
          if (!is_tile_in_bounds(game, neighbor)) continue;
          set_tile_attr(game, neighbor, TILE_NEEDS_REDRAW, true);
        }
      }

      bool was_blocked = get_tile_attr(game, coords, TILE_WAS_BLOCKED);
      bool is_blocked = get_tile_attr(game, coords, TILE_BLOCKED);
      if (is_blocked != was_blocked) {
        set_tile_attr(game, coords, TILE_OVERLAY_NEEDS_REDRAW, true);
      }
      set_tile_attr(game, coords, TILE_WAS_BLOCKED, is_blocked);
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
      if (!get_tile_attr(game, coords, TILE_NEEDS_REDRAW)) continue;
      wxRect tile_rect = this->get_tile_rect(coords);
      if (!update_region.Intersects(tile_rect)) continue;
      set_tile_attr(game, coords, TILE_NEEDS_REDRAW, false);
      // The next layer of the board has to be repainted as well.
      set_tile_attr(game, coords, TILE_OVERLAY_NEEDS_REDRAW, true);

      int tile = get_tile(game, coords);
      wxPoint tile_pos = tile_rect.GetPosition();

      uint32_t coords_hash = fnv32_hash(FNV32_INITIAL_STATE, &coords, sizeof(Coords));

      if (is_water_tile(tile)) {
        this->draw_bitmap(
          dc, tileset.water_tiles[coords_hash % WXSIZEOF(tileset.water_tiles)], tile_pos
        );
        continue;
      }

      this->draw_bitmap(
        dc, tileset.ice_tiles[coords_hash % WXSIZEOF(tileset.ice_tiles)], tile_pos
      );

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
      if (!get_tile_attr(game, coords, TILE_OVERLAY_NEEDS_REDRAW)) continue;
      wxRect tile_rect = this->get_tile_rect(coords);
      if (!update_region.Intersects(tile_rect)) continue;
      set_tile_attr(game, coords, TILE_OVERLAY_NEEDS_REDRAW, false);

      int tile = get_tile(game, coords);
      wxPoint tile_pos = tile_rect.GetPosition();
      dc.Blit(tile_pos, tile_rect.GetSize(), &tiles_dc, tile_pos);

      if (get_tile_attr(game, coords, TILE_BLOCKED)) {
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

void CanvasPanel::paint_selected_tile_outline(wxDC& dc, Coords coords, bool blocked) {
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.SetPen(wxPen(blocked ? *wxRED : *wxGREEN, 5));
  dc.DrawRectangle(this->get_tile_rect(coords));
}

void CanvasPanel::paint_move_arrow(wxDC& dc, Coords start, Coords end) {
  this->paint_move_arrow(dc, start, end, start, true);
}

void CanvasPanel::paint_move_arrow(wxDC& dc, Coords start, Coords end, Coords fail, bool valid) {
  wxPoint arrow_start = this->get_tile_centre(start);
  wxPoint arrow_fail = this->get_tile_centre(fail);
  wxPoint arrow_end = this->get_tile_centre(end);

  wxSize head_size(8, 8);
  wxPen bg_pen(*wxBLACK, 6);
  wxPen green_pen((*wxGREEN).ChangeLightness(75), 4);
  wxPen red_pen((*wxRED).ChangeLightness(75), 4);

  if (!valid && !coords_same(fail, start)) {
    dc.SetPen(bg_pen);
    dc.DrawLine(arrow_start, arrow_fail);
    dc.SetPen(green_pen);
    dc.DrawLine(arrow_start, arrow_fail);
    dc.SetPen(bg_pen);
    this->paint_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
    dc.DrawLine(arrow_fail, arrow_end);
    this->paint_arrow_head(dc, arrow_fail, arrow_end, head_size);
    dc.SetPen(red_pen);
    this->paint_arrow_head(dc, arrow_start, arrow_fail, head_size, ARROW_HEAD_CROSS);
    dc.DrawLine(arrow_fail, arrow_end);
    this->paint_arrow_head(dc, arrow_fail, arrow_end, head_size);
  } else {
    dc.SetPen(bg_pen);
    dc.DrawLine(arrow_start, arrow_end);
    this->paint_arrow_head(dc, arrow_start, arrow_end, head_size);
    dc.SetPen(valid ? green_pen : red_pen);
    dc.DrawLine(arrow_start, arrow_end);
    this->paint_arrow_head(dc, arrow_start, arrow_end, head_size);
  }
}

void CanvasPanel::paint_arrow_head(
  wxDC& dc, wxPoint start, wxPoint end, wxSize head_size, ArrowHeadType head_type
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

void CanvasPanel::on_any_mouse_event(wxMouseEvent& event) {
  this->prev_mouse_pos = this->mouse_pos;
  this->mouse_pos = event.GetPosition();

  if (!this->mouse_is_down) {
    this->mouse_drag_pos = this->mouse_pos;
  }
  if (event.ButtonDown()) {
    this->mouse_is_down_real = this->mouse_is_down = true;
    this->SetFocus();
  } else if (event.ButtonUp()) {
    this->mouse_is_down_real = this->mouse_is_down = false;
  }

  if (event.Entering()) {
    this->mouse_within_window = true;
  } else if (event.Leaving()) {
    this->mouse_within_window = false;
  }

  GameController* controller = this->game_frame->controller;
  // NOTE: a `switch` is unusable here because the event types are defined as
  // `extern` variables. `switch` in C++ can only work with statically-known
  // constants.
  auto event_type = event.GetEventType();
  if (event_type == wxEVT_LEFT_DOWN) {
    controller->on_mouse_down(event);
  } else if (event_type == wxEVT_MOTION) {
    controller->on_mouse_move(event);
  } else if (event_type == wxEVT_LEFT_UP) {
    controller->on_mouse_up(event);
  } else if (event_type == wxEVT_ENTER_WINDOW) {
    controller->on_mouse_enter_leave(event);
  } else if (event_type == wxEVT_LEAVE_WINDOW) {
    controller->on_mouse_enter_leave(event);
  } else {
    event.Skip();
  }
}
