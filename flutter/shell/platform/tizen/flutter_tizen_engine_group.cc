#include "flutter/shell/platform/tizen/flutter_tizen_engine_group.h"

namespace flutter {

FlutterTizenEngine* FlutterTizenEngineGroup::MakeEngineWithProject(
    const FlutterProjectBundle& project) {
  std::unique_ptr<FlutterTizenEngine> engine =
      std::make_unique<flutter::FlutterTizenEngine>(project);

  static uint32_t engine_id = 0;
  engine->SetId(engine_id++);

  engines_.push_back(std::move(engine));
  return engines_.back().get();
}

FlutterTizenEngine* FlutterTizenEngineGroup::GetEngineSpawner() {
  return engines_[0].get();
}

int FlutterTizenEngineGroup::GetEngineCount() {
  return engines_.size();
}

void FlutterTizenEngineGroup::StopEngine(uint32_t id) {
  for (auto it = engines_.begin(); it != engines_.end(); ++it) {
    if (id == it->get()->id()) {
      engines_.erase(it);
      return;
    }
  }
}

}  // namespace flutter