#include "gui/tileset.hh"
#include "gui/canvas.hh"
#include "resources_tileset_png.h"
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/gdicmn.h>
#include <wx/image.h>
#include <wx/mstream.h>

TilesetHelper::TilesetHelper() : scaling(CanvasPanel::TILE_SIZE / TilesetHelper::TILE_SIZE) {}

void TilesetHelper::load() {
  wxMemoryInputStream stream(resources_tileset_png, resources_tileset_png_size);
  this->image = wxImage(stream, wxBITMAP_TYPE_PNG);
  wxASSERT(this->image.IsOk());
  wxSize image_size = this->image.GetSize();
  this->image.Rescale(image_size.x * scaling, image_size.y * scaling, wxIMAGE_QUALITY_NEAREST);

  this->transparent_tile = get_tile(4, 1);
  for (int i = 0; i < int(WXSIZEOF(this->ice_tiles)); i++) {
    this->ice_tiles[i] = get_tile(0 + i % 3, 0 + i / 3);
  }
  for (int i = 0; i < int(WXSIZEOF(this->water_tiles)); i++) {
    this->water_tiles[i] = get_tile(0 + i % 3, 2 + i / 3);
  }
  this->tile_edges[EDGE_TOP] = get_tile(4, 2);
  this->tile_edges[EDGE_RIGHT] = get_tile(3, 1);
  this->tile_edges[EDGE_BOTTOM] = get_tile(4, 0);
  this->tile_edges[EDGE_LEFT] = get_tile(5, 1);
  this->tile_concave_corners[CORNER_TOP_RIGHT] = get_tile(3, 2);
  this->tile_concave_corners[CORNER_BOTTOM_RIGHT] = get_tile(3, 0);
  this->tile_concave_corners[CORNER_BOTTOM_LEFT] = get_tile(5, 0);
  this->tile_concave_corners[CORNER_TOP_LEFT] = get_tile(5, 2);
  this->tile_convex_corners[CORNER_TOP_RIGHT] = get_tile(7, 1);
  this->tile_convex_corners[CORNER_BOTTOM_RIGHT] = get_tile(7, 2);
  this->tile_convex_corners[CORNER_BOTTOM_LEFT] = get_tile(6, 2);
  this->tile_convex_corners[CORNER_TOP_LEFT] = get_tile(6, 1);
  this->blocked_tile = get_tile(7, 0);
  this->grid_tile = get_tile(6, 0);
  for (int i = 0; i < int(WXSIZEOF(this->fish_sprites)); i++) {
    this->fish_sprites[i] = get_tile(5 + i, 3);
  }
  for (int i = 0; i < int(WXSIZEOF(this->penguin_sprites)); i++) {
    auto tile = get_tile(0 + i, 3);
    this->penguin_sprites[i] = tile;
    this->penguin_sprites_flipped[i] = tile.Mirror(/* horizontally */ true);
  }
  this->current_penguin_overlay = get_tile(1, 4);
}
