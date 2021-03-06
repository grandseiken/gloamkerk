#ifndef GLOAM_COMMON_SRC_CORE_COLLISION_H
#define GLOAM_COMMON_SRC_CORE_COLLISION_H
#include "common/src/common/hashes.h"
#include "common/src/core/geometry.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <schema/chunk.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace gloam {
namespace core {
class TileMap;

// 2D edges are in clockwise order (i.e. outward-facing normals on the left).
struct Edge {
  glm::vec2 a;
  glm::vec2 b;
};

class Collision {
public:
  Collision(const TileMap& tile_map);
  // Recalculate terrain geometry from the tile map.
  void update();

  // Project a collision box at position along the given XZ projection vector, and return the
  // modified, unimpeded projection.
  glm::vec2 project_xz(const Box& box, const glm::vec3& position,
                       const glm::vec2& projection) const;

  // Get the correct height for a particular collision box.
  float terrain_height(const Box& box, const glm::vec3& position) const;

private:
  // Get the terrain height at a particular point.
  float terrain_height(const glm::vec3& position) const;

  // Layers of world geometry.
  struct LayerData {
    // All the edges in this layer.
    std::vector<Edge> edges;
    // Acceleration structure: map from tile coordinate to buckets of indexes into edge map, giving
    // all edges incident to the tile.
    std::unordered_map<glm::ivec2, std::vector<std::size_t>> tile_lookup;
  };

  const TileMap& tile_map_;
  std::unordered_map<std::uint32_t, LayerData> layers_;
};

}  // ::core
}  // ::gloam

#endif
