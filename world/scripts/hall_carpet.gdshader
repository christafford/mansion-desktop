shader_type spatial;
// Fine crossed wool yarn, antialiased by distance; all coordinates are meters.
varying vec3 world_position;
void vertex() { world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz; }
void fragment() {
    vec2 p = world_position.xz;
    float grain = fract(sin(dot(floor(p * 650.0), vec2(12.9898, 78.233))) * 43758.5453);
    float weave = sin(p.x * 1800.0) * sin(p.y * 1800.0);
    float fade = 1.0 - smoothstep(0.001, 0.004, max(length(dFdx(p)), length(dFdy(p))));
    float nap = sin(p.x * 31.0 + sin(p.y * 17.0)) * sin(p.y * 27.0);
    ALBEDO = vec3(0.245, 0.028, 0.044) * (0.94 + 0.025 * nap + fade * (0.08 * grain + 0.035 * weave));
    ROUGHNESS = 0.98;
}
