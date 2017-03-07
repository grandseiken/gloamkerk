#include <improbable/standard_library.h>
#include <improbable/worker.h>
#include <schema/gloamkirk.h>
#include <iostream>
#include <string>

namespace {

const improbable::WorkerAttribute kMasterAttribute = {{"master"}};
const improbable::WorkerRequirementSet kMasterOnly = {{{{kMasterAttribute}}}};

std::unordered_map<worker::EntityId, worker::SnapshotEntity> generate() {
  std::unordered_map<worker::EntityId, worker::SnapshotEntity> snapshot;

  worker::Map<worker::ComponentId, improbable::WorkerRequirementSet> acl_map = {
      {gloam::schema::MasterSeed::ComponentId, kMasterOnly}};
  improbable::EntityAclData entity_acl{{kMasterOnly}, {{acl_map}}};

  auto& master_seed_entity = snapshot[0];
  master_seed_entity.Prefab = "MasterSeedEntity";
  master_seed_entity.Add<gloam::schema::Position>({{0., 0., 0.}});
  master_seed_entity.Add<gloam::schema::MasterSeed>({});
  master_seed_entity.Add<improbable::EntityAcl>(entity_acl);

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
