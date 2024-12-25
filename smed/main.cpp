#include <SDL3/SDL_keyboard.h>

#include "omega/core/core.hpp"
#include "omega/events/event.hpp"
#include "omega/gfx/frame_buffer.hpp"
#include "omega/gfx/gl.hpp"
#include "omega/scene/orthographic_camera.hpp"
#include "omega/ui/font_characters.hpp"
#include "omega/util/std.hpp"
#include "smed/editor.hpp"
#include "smed/font.hpp"
#include "smed/lexer.hpp"

using namespace omega;

struct App : public core::App {
    App(const core::AppConfig &config) : core::App(config) {}

    void setup() override {
        gfx::enable_blending();
        cam = util::create_uptr<scene::OrthographicCamera>(
            0.0f, 1600.0f, 0.0f, 900.0f, -1.0f, 1.0f);

        globals->asset_manager.load_shader("font", "./res/shaders/font.glsl");

        globals->asset_manager.load_font("font",
                                         "./res/font/press2p.png",
                                         ui::font_characters::press_start_2p,
                                         8);
        SDL_StartTextInput(window->get_native_window());
        Font::init();
        font = util::create_uptr<Font>(
            "./res/font/FiraMonoNerdFontMono-Regular.otf", 64);

        std::ifstream ifs("./smed/editor.cpp");
        std::string content((std::istreambuf_iterator<char>(ifs)),
                            (std::istreambuf_iterator<char>()));

        editor = util::create_uptr<Editor>(
            globals->asset_manager.get_shader("font"), font.get(), content);

        std::vector<gfx::FrameBufferAttachment> attachments;
        attachments.push_back({
            .width = 1600,
            .height = 900,
            .name = "buffer",
            .min_filter = gfx::texture::TextureParam::NEAREST,
            .mag_filter = gfx::texture::TextureParam::NEAREST,
        });
        fbo = util::create_uptr<gfx::FrameBuffer>(1600, 900, attachments);
    }

    ~App() {
        Font::quit();
    }

    void render(f32 dt) override {
        gfx::set_clear_color(0.1f, 0.05f, 0.14f, 1.0f);
        gfx::clear_buffer(OMEGA_GL_COLOR_BUFFER_BIT);
        cam->recalculate_view_matrix();

        // ui::Font *font = globals->asset_manager.get_font("font");
        auto &batch = globals->sprite_batch;
        auto &shape = globals->shape_renderer;

        // batch.set_view_projection_matrix(cam->get_view_projection_matrix());
        // batch.begin_render();
        editor->render(font.get(), *cam, globals->shape_renderer);
        // batch.end_render();
    }

    void update(f32 dt) override {}

    void input(f32 dt) override {
        auto &keys = globals->input.key_manager;
        if (keys[events::Key::k_escape]) {
            running = false;
        }
        editor->handle_input(globals->input);
    }

    void frame() override {
        f32 dt = tick();

        auto &input = globals->input;
        input.prepare_for_update();

        events::Event event;
        while (input.poll_events(event)) {
            // if (imgui) {
            // ImGui_ImplSDL2_ProcessEvent(&event);
            // }
            switch ((events::EventType)event.type) {
                case events::EventType::quit:
                    running = false;
                    break;
                case events::EventType::window_resized:
                    // change window width, height data
                    core::Window::instance()->on_resize(event.window.data1,
                                                        event.window.data2);
                    on_resize(event.window.data1, event.window.data2);
                    break;
                case events::EventType::mouse_wheel:
                    input.mouse.scroll_wheel =
                        math::vec2((f32)event.wheel.x, (f32)event.wheel.y);
                    break;
                case events::EventType::text_input:
                    editor->handle_text(event.text.text[0]);
                    break;
                case events::EventType::text_editing:
                    util::info("{} {} {} {}",
                               event.edit.text,
                               event.edit.type,
                               event.edit.length,
                               event.edit.timestamp);
                    break;
                default:
                    break;
            }
        }
        input.update();
        // perform the input, update, and render
        this->input(dt);
        this->update(dt);

        // begin_imgui_frame();
        this->render(dt);
        // end_imgui_frame(window);

        window->swap_buffers();
    }

    util::uptr<scene::OrthographicCamera> cam = nullptr;
    util::uptr<Editor> editor = nullptr;
    util::uptr<Font> font = nullptr;
    util::uptr<gfx::FrameBuffer> fbo = nullptr;
};

int main() {
    core::AppConfig config = core::AppConfig::from_config("./res/config.toml");
    App app(config);
    app.run();
    return 0;
}
