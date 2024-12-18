#shader vertex
#version 450
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_tex_coords;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_tex_coords;
layout(location = 1) out vec4 v_color;

uniform mat4 u_view_proj;

void main() {
    gl_Position = u_view_proj * vec4(a_pos, 0., 1.);
    v_tex_coords = a_tex_coords;
    v_color = a_color;
}

#shader fragment
#version 450

layout(location = 0) in vec2 v_tex_coords;
layout(location = 1) in vec4 v_color;

out vec4 color;

uniform sampler2D u_texture;

void main() {
    float r = texture(u_texture, v_tex_coords).r;
    if (r == 0.) {
        discard;
    } else {
        color = vec4(vec3(r), 1.) * v_color;
    }
}
