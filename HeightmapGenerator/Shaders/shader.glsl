#version 330

uniform sampler2D src;          // текущая карта высот
uniform sampler2D source_lines; // изначальные линии
in vec2 uv;
out float frag;

void main() {
    vec2 texel = 1.0 / textureSize(src, 0);

    float original = texture(source_lines, uv).r;

    // Если в исходной карте задана линия — используем только её, без изменений
    if (original > 0.01) {
        frag = original;
        return;
    }

    float sum = 0.0;
    float count = 0.0;

    for (int dx = -1; dx <= 1; ++dx)
    for (int dy = -1; dy <= 1; ++dy) {
        if (dx == 0 && dy == 0) continue;

        vec2 offset = vec2(dx, dy) * texel;
        vec2 neighbor_uv = uv + offset;

        if (neighbor_uv.x < 0.0 || neighbor_uv.x > 1.0 ||
            neighbor_uv.y < 0.0 || neighbor_uv.y > 1.0) {
            continue;
        }

        float neighbor_value = texture(src, neighbor_uv).r;
        float neighbor_original = texture(source_lines, neighbor_uv).r;

        if (neighbor_value > 0.01 || neighbor_original > 0.01) {
            sum += max(neighbor_value, neighbor_original);
            count += 1.0;
        }
    }


    // Обновляем только если были подходящие соседи
    frag = (count > 0.0) ? (sum / count) : 0.0;
}
