#include "workers/client/src/connect_mode.h"
#include "workers/client/src/logic/world.h"
#include <glm/vec4.hpp>
#include <schema/master.h>
#include <schema/player.h>
#include <iostream>

namespace gloam {

ConnectMode::ConnectMode(const std::string& disconnect_reason)
: connected_{false}, disconnect_reason_{disconnect_reason} {}

ConnectMode::ConnectMode(worker::Connection&& connection)
: connection_{new worker::Connection{std::move(connection)}} {
  dispatcher_.OnDisconnect([this](const worker::DisconnectOp& op) {
    std::cerr << "[disconnected] " << op.Reason << std::endl;
    connected_ = false;
    disconnect_reason_ = op.Reason;
  });

  dispatcher_.OnLogMessage([this](const worker::LogMessageOp& op) {
    switch (op.Level) {
    case worker::LogLevel::kDebug:
      std::cout << "[debug] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kInfo:
      std::cout << "[info] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kWarn:
      std::cerr << "[warning] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kError:
      std::cerr << "[error] " << op.Message << std::endl;
      break;
    case worker::LogLevel::kFatal:
      std::cerr << "[fatal] " << op.Message << std::endl;
      connected_ = false;
    default:
      break;
    }
  });

  dispatcher_.OnMetrics([this](const worker::MetricsOp& op) {
    auto metrics = op.Metrics;
    connection_->SendMetrics(metrics);
  });

  dispatcher_.OnCommandResponse<schema::Master::Commands::ClientHeartbeat>(
      [&](const worker::CommandResponseOp<schema::Master::Commands::ClientHeartbeat>& op) {
        if (op.StatusCode != worker::StatusCode::kSuccess) {
          std::cerr << "[warning] Heartbeat failed: " << op.Message << std::endl;
        }
      });

  dispatcher_.OnAuthorityChange<schema::Player>([&](const worker::AuthorityChangeOp& op) {
    if (op.HasAuthority) {
      player_id_ = op.EntityId;
    } else if (op.EntityId == player_id_) {
      player_id_ = -1;
    }
    if (world_) {
      world_->set_player_id(player_id_);
    }
  });

  if (!connection_->IsConnected()) {
    return;
  }
  connection_->SendLogMessage(worker::LogLevel::kInfo, "client", "Connected.");
  world_.reset(new logic::World{*connection_, dispatcher_});
}

ModeResult ConnectMode::event(const sf::Event& event) {
  if (!connected_ && event.type == sf::Event::KeyPressed) {
    disconnect_ack_ = true;
  }
  return {ModeAction::kNone, {}};
}

ModeResult ConnectMode::update() {
  if (connected_) {
    dispatcher_.Process(connection_->GetOpList(/* millis */ 0));

    // Send heartbeat periodically to master.
    if (frame_++ % 512 == 0) {
      connection_->SendCommandRequest<schema::Master::Commands::ClientHeartbeat>(
          /* master entity */ 0, {}, {});
    }
    if (player_id_ >= 0 && frame_ % 64 == 0) {
      logged_in_ = true;
    }
  } else if (disconnect_reason_.empty() || disconnect_ack_) {
    return {ModeAction::kExitToTitle, {}};
  }
  return {{}, {}};
}

void ConnectMode::render(const Renderer& renderer) const {
  auto dimensions = renderer.framebuffer_dimensions();
  if (!connected_) {
    auto text_width = renderer.text_width(disconnect_reason_);
    renderer.draw_text(disconnect_reason_, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, 1.f});
    return;
  } else if (!logged_in_) {
    auto fade = (frame_ % 64) / 64.f;
    std::string text = "ENTERING WORLD...";
    auto text_width = renderer.text_width(text);
    renderer.draw_text(text, {dimensions.x / 2 - text_width / 2, dimensions.y / 2},
                       glm::vec4{.75f, .75f, .75f, fade});
  } else {
    world_->render(renderer);
  }
}

}  // ::gloam
