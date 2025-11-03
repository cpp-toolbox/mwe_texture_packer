#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <iostream>

#include "graphics/shader_standard/shader_standard.hpp"

#include "graphics/texture_packer/texture_packer.hpp"

#include "utility/texture_packer_model_loading/texture_packer_model_loading.hpp"
#include "utility/resource_path/resource_path.hpp"

#include "system_logic/toolbox_engine/toolbox_engine.hpp"

int main() {

    // global_logger.add_file_sink("logs.txt");

    std::unordered_map<SoundType, std::string> sound_type_to_file = {
        {SoundType::UI_HOVER, "assets/sounds/hover.wav"},
        {SoundType::UI_CLICK, "assets/sounds/click.wav"},
        {SoundType::UI_SUCCESS, "assets/sounds/success.wav"},
    };

    ToolboxEngine tbx_engine(
        "mwe_texture_packer",
        {ShaderType::ABSOLUTE_POSITION_WITH_COLORED_VERTEX, ShaderType::TEXTURE_PACKER_CWL_V_TRANSFORMATION_UBOS_1024},
        sound_type_to_file);

    global_logger.remove_all_sinks();

    const auto textures_directory = std::filesystem::path("assets");
    std::filesystem::path output_dir = std::filesystem::path("assets") / "packed_textures";
    int container_side_length = 4096;

    TexturePacker texture_packer(textures_directory, output_dir, container_side_length);

    // TODO: I don't like having this thing around
    ResourcePath rp(false);

    tbx_engine.shader_cache.set_uniform(ShaderType::TEXTURE_PACKER_CWL_V_TRANSFORMATION_UBOS_1024,
                                        ShaderUniformVariable::PACKED_TEXTURE_BOUNDING_BOXES, 1);

    auto lightbulb = texture_packer_model_loading::parse_model_into_tig(
        // rp, "assets/models/lightbulb/lightbulb.obj", texture_packer,
        rp, "assets/models/spider_crossings/spider_crossings.obj", texture_packer,
        // rp, "assets/models/dm_turbine/dm_turbine.obj", texture_packer,
        tbx_engine.batcher.texture_packer_cwl_v_transformation_ubos_1024_shader_batcher.object_id_generator,
        tbx_engine.batcher.texture_packer_cwl_v_transformation_ubos_1024_shader_batcher.ltw_object_id_generator);

    tbx_engine.fps_camera.unfreeze_camera();
    tbx_engine.window.disable_cursor();

    auto tick = [&](double dt) {

        auto [arx, ary] = tbx_engine.window.get_aspect_ratio_in_simplest_terms();

        tbx_engine.shader_cache.set_uniform(ShaderType::ABSOLUTE_POSITION_WITH_COLORED_VERTEX,
                                        ShaderUniformVariable::ASPECT_RATIO, glm::vec2(arx, ary));

        tbx_engine.shader_cache.set_uniform(ShaderType::TEXTURE_PACKER_CWL_V_TRANSFORMATION_UBOS_1024,
                                            ShaderUniformVariable::CAMERA_TO_CLIP,
                                            tbx_engine.fps_camera.get_projection_matrix());

        tbx_engine.shader_cache.set_uniform(ShaderType::TEXTURE_PACKER_CWL_V_TRANSFORMATION_UBOS_1024,
                                            ShaderUniformVariable::WORLD_TO_CAMERA,
                                            tbx_engine.fps_camera.get_view_matrix());

        tbx_engine.update_camera_position_with_default_movement(dt);

        tbx_engine.draw_chosen_engine_stats();
        tbx_engine.process_and_queue_render_input_graphics_sound_menu();
        tbx_engine.batcher.texture_packer_cwl_v_transformation_ubos_1024_shader_batcher.queue_draw(lightbulb);

        tbx_engine.batcher.texture_packer_cwl_v_transformation_ubos_1024_shader_batcher.draw_everything();
        tbx_engine.batcher.texture_packer_cwl_v_transformation_ubos_1024_shader_batcher.upload_ltw_matrices();

        tbx_engine.batcher.absolute_position_with_colored_vertex_shader_batcher.draw_everything();
        tbx_engine.sound_system.play_all_sounds();
        TemporalBinarySignal::process_all();
    };

    auto term = [&]() { return tbx_engine.window.window_should_close(); };

    tbx_engine.main_loop.start(tbx_engine.window.wrap_tick_with_required_glfw_calls(tick), term);

    return 0;
}
