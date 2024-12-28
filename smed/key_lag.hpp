#ifndef SMED_KEYLAG_HPP
#define SMED_KEYLAG_HPP

#include <functional>

#include "omega/events/event.hpp"
#include "omega/events/input_manager.hpp"

void register_key(omega::events::Key key, std::function<void()> callback);
void update_keys(omega::events::InputManager &input);

#endif // SMED_KEYLAG_HPP
