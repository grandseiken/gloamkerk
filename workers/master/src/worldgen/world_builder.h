#ifndef GLOAM_WORKERS_MASTER_SRC_WORLDGEN_WORLD_BUILDER_H
#define GLOAM_WORKERS_MASTER_SRC_WORLDGEN_WORLD_BUILDER_H
#include <glm/vec2.hpp>
#include <cstdlib>
#include <memory>
#include <random>

namespace gloam {
namespace schema {
class ChunkData;
}

namespace master {
namespace worldgen {
class MapBuilder;

enum class WorldType {
  kDefault,
};

class Random {
public:
  Random(std::size_t seed);

  bool get_bool();
  std::int32_t get_int(std::uint32_t max);
  std::int32_t get_int(std::int32_t min, std::int32_t max);

private:
  std::mt19937 generator;
};

struct Map {
  glm::ivec2 origin;
  std::unique_ptr<MapBuilder> builder;
};

class WorldBuilder {
public:
  WorldBuilder(WorldType type);
  std::vector<glm::ivec2> get_chunks(std::int32_t chunk_size) const;
  schema::ChunkData get_chunk_data(std::int32_t chunk_size, const glm::ivec2& chunk) const;

private:
  WorldType type_;
  std::vector<Map> maps_;
};

}  // ::worldgen
}  // ::master
}  // ::gloam

#endif