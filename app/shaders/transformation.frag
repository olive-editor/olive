uniform sampler2D tex_in;

uniform float u_translation_x;
uniform float u_translation_y;
uniform float u_rotation;
uniform float u_scale;
uniform vec2 resolution_in;

in vec2 ove_texcoord;
out vec4 frag_color;

#define PI 3.14159265358979323846

void main() {
    float aspectRatio = resolution_in.x / resolution_in.y;
    
    vec2 uvAspectCorrected = vec2(
        (ove_texcoord.x - 0.5) * aspectRatio + 0.5,
        ove_texcoord.y
    );

    uvAspectCorrected = (uvAspectCorrected - 0.5) / (u_scale * 0.01) + 0.5;

    float rotation = u_rotation * PI / 180.0;

    mat2 rotationMatrix = mat2(
        cos(rotation), -sin(rotation),
        sin(rotation),  cos(rotation)
    );

    vec2 uvRotated = (uvAspectCorrected - 0.5) * rotationMatrix + 0.5;

    uvAspectCorrected = vec2(
        (uvRotated.x - 0.5) * (1.0 / aspectRatio) + 0.5,
        uvRotated.y
    );

    vec2 uvTranslated = uvAspectCorrected + vec2(u_translation_x * 0.02, u_translation_y * 0.02);

    frag_color = texture(tex_in, uvTranslated);
}
