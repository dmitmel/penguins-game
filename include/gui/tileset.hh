#pragma once

#include <wx/bitmap.h>
#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/image.h>

enum TileEdge {
  EDGE_TOP = 0,
  EDGE_RIGHT = 1,
  EDGE_BOTTOM = 2,
  EDGE_LEFT = 3,
  EDGE_MAX,
};

enum TileCorner {
  CORNER_TOP_RIGHT = 0,
  CORNER_BOTTOM_RIGHT = 1,
  CORNER_BOTTOM_LEFT = 2,
  CORNER_TOP_LEFT = 3,
  CORNER_MAX,
};

class TilesetHelper {
  wxDECLARE_NO_COPY_CLASS(TilesetHelper);

public:
  TilesetHelper(int scaling);

  void load();

  wxImage get_sub_image(const wxRect& rect) const {
    return this->get_sub_image(rect.x, rect.y, rect.width, rect.height);
  }
  wxImage get_sub_image(int x, int y, int w, int h) const {
    wxRect rect(x * scaling, y * scaling, w * scaling, h * scaling);
    return this->image.GetSubImage(rect);
  }

  wxImage get_tile(const wxPoint& point) const {
    return this->get_tile(point.x, point.y);
  }
  wxImage get_tile(int x, int y) const {
    return this->get_sub_image(x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
  }

  static const int TILE_SIZE = 20;

  const int scaling;
  wxImage image;

  wxBitmap water_tiles[3];
  wxBitmap ice_tiles[6];
  wxBitmap tile_edges[EDGE_MAX];
  wxBitmap tile_convex_corners[CORNER_MAX];
  wxBitmap tile_concave_corners[CORNER_MAX];
  wxBitmap blocked_tile;
  wxBitmap grid_tile;
  wxBitmap fish_sprites[3];
  wxBitmap penguin_sprites[5];
  wxBitmap penguin_sprites_flipped[WXSIZEOF(penguin_sprites)];
  wxBitmap current_penguin_overlay;
};
