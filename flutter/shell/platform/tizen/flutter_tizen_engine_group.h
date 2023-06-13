#ifndef EMBEDDER_FLUTTER_TIZEN_ENGINE_GROUP_H_
#define EMBEDDER_FLUTTER_TIZEN_ENGINE_GROUP_H_

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

namespace flutter {

class FlutterTizenEngineGroup {
 public:
  static FlutterTizenEngineGroup& GetInstance() {
    static FlutterTizenEngineGroup instance;
    return instance;
  }

  FlutterTizenEngine* GetEngineSpawner();

  FlutterTizenEngine* MakeEngineWithProject(
      const FlutterProjectBundle& project);

  int GetEngineCount();

  void StopEngine(uint32_t id);

 private:
  FlutterTizenEngineGroup() {}

  std::vector<std::unique_ptr<FlutterTizenEngine>> engines_;
};

}  // namespace flutter

#endif