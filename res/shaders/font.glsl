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

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0);
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

void main() {
    float r = texture(u_texture, v_tex_coords).r;
    if (r == 0.) {
        discard;
    } else {
        // float sin_t = sin(u_time);
        color = vec4(vec3(r), 1.) * v_color;
        color.a = 1.0;
        // color.rgb = hsl2rgb(vec3((u_time + v_uv.x + v_uv.y), 0.5, 0.5));
        // color.rgb *= v_color.rgb;
        // color.r += sin_t * 0.2;
        // color.g -= v_uv.x * sin_t * 0.2;
        // color.b += v_uv.y * sin_t * 0.4;
        // color.b *= color.b;
    }
}
