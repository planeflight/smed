#shader vertex
#version 450
layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_tex_coords;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_tex_coords;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec4 v_color;

uniform mat4 u_view_proj;

void main() {
    gl_Position = u_view_proj * vec4(a_pos, 0., 1.);
    v_tex_coords = a_tex_coords;
    v_color = a_color;
    v_uv = a_pos / vec2(1600., 900.);
}

#shader fragment
#version 450

layout(location = 0) in vec2 v_tex_coords;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec4 v_color;

out vec4 color;

uniform sampler2D u_texture;
uniform float u_time;

void main() {
    float r = texture(u_texture, v_tex_coords).r;
    if (r == 0.) {
        discard;
    } else {
        float sin_t = sin(u_time + v_uv.x * 10.0);
        float cos_t = cos(u_time + v_uv.y * 10.0);
        color = vec4(vec3(r), 1.) * v_color;
        if (v_color == vec4(1.0)) {
            color.rgb = vec3(
                0.3 + sin_t * 0.5 + 0.5,
                0.4 + cos_t * 0.5 + 0.5,
                0.4
            );
            color.a = 1.0;
        }
    }
}
