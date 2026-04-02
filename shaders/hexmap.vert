#version 450
layout(location = 0) in vec2 pos;
layout(location = 1) in vec3 colorIn;
layout(location = 0) out vec3 color;

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

void main() {
    gl_Position = pc.model * vec4(pos, 0.0, 1.0);
    color = colorIn;
}
