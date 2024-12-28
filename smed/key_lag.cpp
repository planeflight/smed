#include "key_lag.hpp"

#include <vector>

#include "omega/util/time.hpp"

struct KeyLag {
    omega::events::Key key;
    std::function<void()> callback = nullptr;
    f32 lag_time = 0.0f; // track when the key was initially pressed
    u32 slow_timer = 0;
};
static std::vector<KeyLag> key_list;

void register_key(omega::events::Key key, std::function<void()> callback) {
    key_list.push_back({key, callback});
}

void update_keys(omega::events::InputManager &input) {
    auto &keys = input.key_manager;
    for (auto &k : key_list) {
        if (keys.key_just_pressed(k.key)) {
            k.slow_timer++;
            k.callback();
            k.lag_time = omega::util::time::get_time<f32>();
        } else if (keys[k.key]) {
            f32 new_time = omega::util::time::get_time<f32>();
            f32 diff = new_time - k.lag_time;
            k.slow_timer++;
            // makes the key actually happen every other frame
            if (diff > 0.45f && k.slow_timer % 2 == 0) {
                k.callback();
            }
        }
        // otherwise reset the cooldown timer
        else {
            k.slow_timer = 0;
        }
    }
}
