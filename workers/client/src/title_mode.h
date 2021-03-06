#ifndef GLOAM_WORKERS_CLIENT_SRC_TITLE_MODE_H
#define GLOAM_WORKERS_CLIENT_SRC_TITLE_MODE_H
#include "workers/client/src/glo.h"
#include "workers/client/src/mode.h"
#include "workers/client/src/renderer.h"
#include <improbable/worker.h>
#include <cstdint>
#include <memory>

namespace gloam {

class TitleMode : public Mode {
public:
  TitleMode(ModeState& mode_state, bool fade_in,
            const worker::ConnectionParameters& connection_params,
            const worker::LocatorParameters& locator_params);
  void tick(const Input& input) override;
  void sync() override {}
  void render(const Renderer& renderer) const override;

private:
  ModeState& mode_state_;
  std::uint64_t fade_in_ = 0;
  std::uint64_t connect_frame_ = 0;
  std::uint64_t finish_connect_frame_ = 0;

  bool connection_cancel_ = false;
  worker::ConnectionParameters connection_params_;
  worker::Locator locator_;
  // Workaround for broken future move-constructor.
  struct LocatorFutureWrapper {
    worker::Future<worker::DeploymentList> future;
  };
  struct ConnectionFutureWrapper {
    worker::Future<worker::Connection> future;
  };
  std::int32_t deployment_choice_ = 0;
  std::unique_ptr<worker::DeploymentList> deployment_list_;
  std::string queue_status_;
  std::string new_queue_status_;
  std::string queue_status_error_;

  std::unique_ptr<LocatorFutureWrapper> locator_future_;
  std::unique_ptr<ConnectionFutureWrapper> connection_future_;

  TextureImage title_;
  glo::Program title_program_;
};

}  // ::gloam

#endif
