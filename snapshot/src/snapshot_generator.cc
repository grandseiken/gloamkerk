#include "common/src/common/definitions.h"
#include <improbable/standard_library.h>
#include <improbable/worker.h>
#include <schema/master.h>
#include <iostream>
#include <string>

namespace {
std::unordered_map<worker::EntityId, worker::Entity> generate() {
  std::unordered_map<worker::EntityId, worker::Entity> snapshot;
  improbable::EntityAclData entity_acl{
      gloam::common::kMasterOnlySet,
      {{gloam::schema::Master::ComponentId, gloam::common::kMasterOnlySet}}};

  auto& master_seed_entity = snapshot[gloam::common::kMasterSeedEntityId];
  master_seed_entity.Add<gloam::schema::Master>({false, {}});
  master_seed_entity.Add<improbable::EntityAcl>(entity_acl);
  master_seed_entity.Add<improbable::Metadata>({gloam::common::kMasterSeedEntityType});
  master_seed_entity.Add<improbable::Persistence>({});
  master_seed_entity.Add<improbable::Position>({{0., 0., 0.}});

  return snapshot;
}

}  // anonymous

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "[error] Usage: " << argv[0] << " output/path" << std::endl;
    return 1;
  }

  auto snapshot = generate();
  auto error = worker::SaveSnapshot(argv[1], snapshot);
  if (error) {
    std::cerr << "[error] " << *error << std::endl;
    return 1;
  }
  std::cout << "[info] Wrote " << argv[1] << " with " << snapshot.size() << " entities."
            << std::endl;
  return 0;
}
