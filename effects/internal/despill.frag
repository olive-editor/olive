/***
    Despill - for cleaning up keying residue.
    Written by unfa for Olive Video Editor.
***/

uniform sampler2D tex;
varying vec2 uTexCoord;
uniform vec2 resolution;
uniform int channel;
uniform float factor;

const vec2 renderScale = vec2(1,1);

void main(void)
{
    
    factor = factor * 0.01; // convert factor from 0-100 scale to 0-1 scale
    
    vec2 uv = gl_FragCoord.xy/resolution.xy;

    vec4 col = texture2D(tex, uv);
    
    // separate channels
    
    float r = col.r;
    float g = col.g;
    float b = col.b;
    float a = col.a;
    
    // Select despilled channel: 0 = RED; 1 = GREEN; 2 = BLUE;
    
    if (channel == 0) // RED
    { 
        // replace new green channel by avaraging between existing red and blue channels
        float r2 = (g + b) * 0.5;
        
        // now composite the original and new gree channel using the "darken" mode
        r = ( min(r, r2) * factor ) + (r * ( 1.0 - factor));
    }
    
    if (channel == 1) // GREEN
    { 
        // replace new green channel by avaraging between existing red and blue channels
        float g2 = (r + b) * 0.5;
        
        // now composite the original and new gree channel using the "darken" mode
        g = ( min(g, g2) * factor ) + (g * ( 1.0 - factor));
    }
    
    if (channel == 2) // BLUE
    { 
        // replace new green channel by avaraging between existing red and blue channels
        float b2 = (r + g) * 0.5;
        
        // now composite the original and new gree channel using the "darken" mode
        b = ( min(b, b2) * factor ) + (b * ( 1.0 - factor));
    }
        

    // and return the result,

    gl_FragColor = vec4(r, g, b, a);
}
