shader_type spatial;
uniform vec4 stone_color : source_color = vec4(0.65, 0.61, 0.51, 1.0);
uniform bool floor_surface = false;
varying vec3 p;
varying vec3 n;
void vertex() {
    p = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    n = normalize(MODEL_NORMAL_MATRIX * NORMAL);
}
void fragment() {
    vec2 uv = abs(n.y) > 0.6 ? p.xz : (abs(n.x) > abs(n.z) ? p.zy : p.xy);
    vec2 scale = floor_surface ? vec2(0.55) : vec2(0.45, 1.2);
    vec2 course = uv * scale;
    if (!floor_surface) course.x += mod(floor(course.y), 2.0) * 0.5;
    vec2 edge = abs(fract(course) - 0.5);
    float footprint = max(fwidth(course.x), fwidth(course.y));
    float joint = smoothstep(0.49 - footprint, 0.499 + footprint, max(edge.x, edge.y));
    float vein = pow(0.5 + 0.5 * sin(uv.x * 3.5 + uv.y * 1.7 + sin(uv.y * 2.8) * 1.3 + sin(uv.x * 8.0) * 0.15), 18.0);
    float grain = sin(dot(uv, vec2(117.3, 233.9))) * sin(dot(uv, vec2(329.1, 173.1)));
    vec3 color = stone_color.rgb * (1.0 - vein * (floor_surface ? 0.13 : 0.045) + grain * 0.006);
    ALBEDO = mix(color, stone_color.rgb * 0.58, joint * 0.6);
    ROUGHNESS = floor_surface ? 0.4 : 0.79;
}
