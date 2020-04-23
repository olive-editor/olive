#version 150

// Custom inputs
uniform float hsv_value;

// Standard inputs
uniform vec2 ove_resolution;

in vec2 ove_texcoord;

out vec4 fragColor;

// Color wheel uses PI
#define M_PI 3.1415926535897932384626433832795

vec3 hsv_to_rgb(float H, float S, float V) {
    float C = S * V;
    float X = C * (1.0 - abs(mod(H / 60.0, 2.0) - 1.0));
    float m = V - C;
    float Rs, Gs, Bs;

    if(H >= 0.0 && H < 60.0) {
        Rs = C;
        Gs = X;
        Bs = 0.0; 
    }
    else if(H >= 60.0 && H < 120.0) {   
        Rs = X;
        Gs = C;
        Bs = 0.0; 
    }
    else if(H >= 120.0 && H < 180.0) {
        Rs = 0.0;
        Gs = C;
        Bs = X; 
    }
    else if(H >= 180.0 && H < 240.0) {
        Rs = 0.0;
        Gs = X;
        Bs = C; 
    }
    else if(H >= 240.0 && H < 300.0) {
        Rs = X;
        Gs = 0.0;
        Bs = C; 
    }
    else {
        Rs = C;
        Gs = 0.0;
        Bs = X; 
    }

    return vec3(Rs + m, Gs + m, Bs + m);
}

void main(void) {
    vec2 center = vec2(0.5, 0.5);
    float radius = 0.5;

    vec2 flipped_texcoord = vec2(ove_texcoord.x, (1.0 - ove_texcoord.y));

    // Calculate scale
    if (ove_resolution.x != ove_resolution.y) {
        // Scale around center
        flipped_texcoord -= vec2(0.5, 0.5);

        if (ove_resolution.x > ove_resolution.y) {
            flipped_texcoord.x *= ove_resolution.x / ove_resolution.y;
        } else {
            flipped_texcoord.y *= ove_resolution.y / ove_resolution.x;
        }
        
        flipped_texcoord += vec2(0.5, 0.5);
    }

    float dist = distance(flipped_texcoord, center);

    if (dist <= radius) {
        float opposite = flipped_texcoord.y - center.y;
        float adjacent = flipped_texcoord.x - center.x;

        float hue = atan(opposite, adjacent) * 180.0 / M_PI + 180.0;
        float sat = dist / radius;

        // Extremely simple antialiasing, we taper off the edges between (radius) and (radius - 1)
        float pixel_radius = min(ove_resolution.x, ove_resolution.y) * 0.5;
        float pixel_dist = (pixel_radius / radius) * dist;
        float alpha = min((pixel_radius - pixel_dist), 1.0);

        fragColor = vec4(hsv_to_rgb(hue, sat, hsv_value), alpha);
    } else {
        fragColor = vec4(0.0);
    }
}
