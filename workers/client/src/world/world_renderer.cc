#include "workers/client/src/world/world_renderer.h"
#include "workers/client/src/renderer.h"
#include "workers/client/src/shaders/common.h"
#include "workers/client/src/shaders/fog.h"
#include "workers/client/src/shaders/light.h"
#include "workers/client/src/shaders/world.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <schema/chunk.h>
#include <algorithm>

namespace gloam {
namespace world {
namespace {
// Tile size in pixels.
const std::int32_t kTileSize = 32;

std::vector<GLushort> generate_indices(GLushort count) {
  std::vector<GLushort> indices;
  GLushort index = 0;
  for (GLushort i = 0; i < count; ++i) {
    indices.push_back(index + 0);
    indices.push_back(index + 2);
    indices.push_back(index + 1);
    indices.push_back(index + 1);
    indices.push_back(index + 2);
    indices.push_back(index + 3);
    index += 4;
  }
  return indices;
}

glo::VertexData generate_world_data(const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) {
  std::vector<float> data;
  GLushort count = 0;

  auto add_vec4 = [&](const glm::vec4& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(v.w);
  };

  for (const auto& pair : tile_map) {
    const auto& coord = pair.first;
    auto min = glm::vec2{coord * kTileSize};
    auto max = glm::vec2{(coord + glm::ivec2{1, 1}) * kTileSize};
    auto height = static_cast<float>(pair.second.height() * kTileSize);
    glm::vec4 top_normal = {0, 1., 0., 1.};

    add_vec4({min.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({min.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, min.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    add_vec4({max.x, height, max.y, 1.f});
    add_vec4(top_normal);
    data.push_back(0);
    ++count;

    auto jt = tile_map.find(coord - glm::ivec2{0, 1});
    if (jt != tile_map.end() && jt->second.height() != pair.second.height()) {
      auto y = coord.y * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.x * kTileSize, next_height};
      max = glm::vec2{(1 + coord.x) * kTileSize, height};
      glm::vec4 side_normal = {0., 0., jt->second.height() > pair.second.height() ? 1. : -1., 1.};

      add_vec4({min.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({min.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, min.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({max.x, max.y, y, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      ++count;
    }

    jt = tile_map.find(coord - glm::ivec2{1, 0});
    if (jt != tile_map.end()) {
      auto x = coord.x * kTileSize;
      auto next_height = static_cast<float>(jt->second.height() * kTileSize);
      min = glm::vec2{coord.y * kTileSize, next_height};
      max = glm::vec2{(1 + coord.y) * kTileSize, height};
      glm::vec4 side_normal = {jt->second.height() > pair.second.height() ? 1. : -1., 0., 0., 1.};

      add_vec4({x, min.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, min.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, min.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      add_vec4({x, max.y, max.x, 1.f});
      add_vec4(side_normal);
      data.push_back(1);
      ++count;
    }
  }

  glo::VertexData result{data, generate_indices(count), GL_DYNAMIC_DRAW};
  result.enable_attribute(0, 4, 9, 0);
  result.enable_attribute(1, 4, 9, 4);
  result.enable_attribute(2, 1, 9, 8);
  return result;
}

glo::VertexData generate_fog_data(const glm::vec3& camera, const glm::vec2& dimensions,
                                  std::int32_t layer) {
  std::vector<float> data;
  GLushort count = 0;

  auto add_vec3 = [&](const glm::vec3& v) {
    data.push_back(v.x);
    data.push_back(v.y);
    data.push_back(v.z);
    data.push_back(1.f);
  };

  auto distance = std::max(dimensions.x, dimensions.y);
  auto centre = camera + glm::vec3{0., (.5 + layer) * static_cast<float>(kTileSize), 0.};
  add_vec3(centre + glm::vec3{-distance, 0, -distance});
  add_vec3(centre + glm::vec3{-distance, 0, distance});
  add_vec3(centre + glm::vec3{distance, 0, -distance});
  add_vec3(centre + glm::vec3{distance, 0, distance});
  ++count;

  glo::VertexData result{data, generate_indices(count), GL_DYNAMIC_DRAW};
  result.enable_attribute(0, 4, 0, 0);
  return result;
}

}  // anonymous

WorldRenderer::WorldRenderer()
: world_program_{"world",
                 {"world_vertex", GL_VERTEX_SHADER, shaders::world_vertex},
                 {"world_fragment", GL_FRAGMENT_SHADER, shaders::world_fragment}}
, material_program_{"material",
                    {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                    {"material_fragment", GL_FRAGMENT_SHADER, shaders::material_fragment}}
, light_program_{"light",
                 {"quad_vertex", GL_VERTEX_SHADER, shaders::quad_vertex},
                 {"light_fragment", GL_FRAGMENT_SHADER, shaders::light_fragment}}
, fog_program_{"fog",
               {"fog_vertex", GL_VERTEX_SHADER, shaders::fog_vertex},
               {"fog_fragment", GL_FRAGMENT_SHADER, shaders::fog_fragment}} {}

void WorldRenderer::render(const Renderer& renderer, const glm::vec3& camera,
                           const std::unordered_map<glm::ivec2, schema::Tile>& tile_map) const {
  auto idimensions = renderer.framebuffer_dimensions();
  if (!world_buffer_ || world_buffer_->dimensions() != idimensions) {
    world_buffer_.reset(new glo::Framebuffer{idimensions});
    // World position buffer.
    world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // World normal buffer.
    world_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // World material buffer.
    world_buffer_->add_colour_buffer(/* RGBA */ false);
    // World depth buffer.
    world_buffer_->add_depth_stencil_buffer();

    material_buffer_.reset(new glo::Framebuffer{idimensions});
    // Normal buffer.
    material_buffer_->add_colour_buffer(/* high-precision RGB */ true);
    // Colour buffer.
    material_buffer_->add_colour_buffer(/* RGBA */ false);

    world_buffer_->check_complete();
    material_buffer_->check_complete();
  }

  // Not sure what exact values we need for z-planes to be correct, this should do for now.
  auto camera_distance = static_cast<float>(std::max(kTileSize, 2 * idimensions.y));
  glm::vec2 dimensions = idimensions;

  auto ortho = glm::ortho(dimensions.x / 2, -dimensions.x / 2, -dimensions.y / 2, dimensions.y / 2,
                          1 / camera_distance, 2 * camera_distance);
  auto look_at = glm::lookAt(camera + camera_distance * glm::vec3{.5f, 1.f, -1.f}, camera,
                             glm::vec3{0.f, 1.f, 0.f});
  auto camera_matrix = ortho * look_at;

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  {
    auto draw = world_buffer_->draw();
    glViewport(0, 0, idimensions.x, idimensions.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto program = world_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
    renderer.set_simplex3_uniforms(program);
    generate_world_data(tile_map).draw();
  }

  renderer.set_default_render_states();
  {
    auto draw = material_buffer_->draw();
    glViewport(0, 0, idimensions.x, idimensions.y);

    auto program = material_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("world_buffer_normal", world_buffer_->colour_textures()[1]);
    program.uniform_texture("world_buffer_material", world_buffer_->colour_textures()[2]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    renderer.set_simplex3_uniforms(program);
    renderer.draw_quad();
  }

  static float a = 0;
  a += 1 / 128.f;
  auto light_position = camera + glm::vec3{256.f * cos(a), 48.f, 256.f * sin(a)};
  {
    // Should be converted to draw the lights as individuals quads in a single draw call.
    auto program = light_program_.use();
    program.uniform_texture("world_buffer_position", world_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_normal", material_buffer_->colour_textures()[0]);
    program.uniform_texture("material_buffer_colour", material_buffer_->colour_textures()[1]);
    glUniform2fv(program.uniform("dimensions"), 1, glm::value_ptr(dimensions));
    glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
    glUniform1f(program.uniform("light_intensity"), 1.f);
    renderer.draw_quad();
  }

  {
    // Copy over the depth value so we can do forward rendering into the final buffer.
    auto read = world_buffer_->read();
    glBlitFramebuffer(0, 0, idimensions.x, idimensions.y, 0, 0, idimensions.x, idimensions.y,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  glm::vec4 fog_colour = {.5, .5, .5, 1. / 4};

  renderer.set_default_render_states();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  auto render_fog = [&](std::int32_t layer) {
    auto program = fog_program_.use();
    glUniformMatrix4fv(program.uniform("camera_matrix"), 1, false, glm::value_ptr(camera_matrix));
    renderer.set_simplex3_uniforms(program);
    glUniform4fv(program.uniform("fog_colour"), 1, glm::value_ptr(fog_colour));
    glUniform3fv(program.uniform("light_world"), 1, glm::value_ptr(light_position));
    glUniform1f(program.uniform("light_intensity"), 1.f);
    glUniform1f(program.uniform("frame"), renderer.frame());
    generate_fog_data(camera, renderer.framebuffer_dimensions(), layer).draw();
  };

  for (int32_t i = -3; i < 3; ++i) {
    // Fog must be rendered in separate draw calls for transparency.
    render_fog(i);
  }
}

}  // ::world
}  // ::gloam
