#ifndef GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_H
#define GLOAM_WORKERS_CLIENT_SRC_WORLD_WORLD_H
#include "common/src/common/hashes.h"
#include "common/src/core/collision.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/world/world_renderer.h"
#include <glm/vec3.hpp>
#include <improbable/worker.h>
#include <schema/chunk.h>
#include <unordered_map>
#include <unordered_set>

namespace worker {
class Connection;
class Dispatcher;
}  // ::worker

namespace gloam {
class Input;
class Renderer;

namespace world {

class World {
public:
  World(worker::Connection& connection_, worker::Dispatcher& dispatcher_,
        const ModeState& mode_state);
  void set_player_id(worker::EntityId player_id);

  // Game loop functions: sync() is called at the network frame-rate; update() is called at the
  // client rendering frame-rate.
  void tick(const Input& input);
  void sync();
  void render(const Renderer& renderer, std::uint64_t frame) const;

private:
  void update_chunk(const schema::ChunkData& data);
  void clear_chunk(const schema::ChunkData& data);

  worker::Connection& connection_;
  worker::Dispatcher& dispatcher_;
  worker::EntityId player_id_;
  std::unordered_set<worker::EntityId> player_entities_;

  std::unordered_map<worker::EntityId, glm::vec3> entity_positions_;
  std::unordered_map<worker::EntityId, schema::ChunkData> chunk_map_;
  std::unordered_map<glm::ivec2, schema::Tile> tile_map_;
  core::Collision collision_;
  WorldRenderer world_renderer_;
  bool tile_map_changed_ = false;
};

}  // ::world
}  // ::gloam

#endif
