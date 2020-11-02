#version 330 core

out vec4 FragColor;

in vec3 vPosition;

// https://github.com/wwwtyro/glsl-atmosphere/blob/master/index.glsl
// START

#define PI 3.141592
#define iSteps 16
#define jSteps 8

vec2 rsi(vec3 r0, vec3 rd, float sr){
  float a = dot(rd, rd);
  float b = 2.0 * dot(rd, r0);
  float c = dot(r0, r0) - (sr * sr);
  float d = (b*b) - 4.0*a*c;
  if(d < 0.0){
    return vec2(1e5,-1e5);
  }

  return vec2((-b - sqrt(d))/(2.0*a), (-b + sqrt(d))/(2.0*a));
}

vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g){
  pSun = normalize(pSun);
  r = normalize(r);

  vec2 p = rsi(r0, r, rAtmos);
  if (p.x > p.y) return vec3(0,0,0);
  p.y = min(p.y, rsi(r0, r, rPlanet).x);
  float iStepSize = (p.y - p.x) / float(iSteps);

  float iTime = 0.0;

  vec3 totalRlh = vec3(0,0,0);
  vec3 totalMie = vec3(0,0,0);

  float iOdRlh = 0.0;
  float iOdMie = 0.0;

  float mu = dot(r, pSun);
  float mumu = mu * mu;
  float gg = g * g;
  float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
  float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

  for(int i = 0; i < iSteps; i++){
    vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);

    float iHeight = length(iPos) - rPlanet;

    float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
    float odStepMie = exp(-iHeight / shMie) * iStepSize;

    iOdRlh += odStepRlh;
    iOdMie += odStepMie;

    float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

    float jTime = 0.0;

    float jOdRlh = 0.0;
    float jOdMie = 0.0;

    for(int j = 0; j < jSteps; j++){
      vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

      float jHeight = length(jPos) - rPlanet;

      jOdRlh += exp(-jHeight / shRlh) * jStepSize;
      jOdMie += exp(-jHeight / shMie) * jStepSize;

      jTime += jStepSize;
    }

    vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

    totalRlh += odStepRlh * attn;
    totalMie += odStepMie * attn;

    iTime += iStepSize;
  }

  return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

// END

const vec3 sun_direction = normalize(vec3(1, 3, 2));

void main(){
  vec3 color = atmosphere(
    normalize(vPosition),           // normalized ray direction
    vec3(0,6372e3,0),               // ray origin
    sun_direction,                  // position of the sun
    100.0,                          // intensity of the sun
    6371e3,                         // radius of the planet in meters
    6471e3,                         // radius of the atmosphere in meters
    vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
    21e-6,                          // Mie scattering coefficient
    8e3,                            // Rayleigh scale height
    1.2e3,                          // Mie scale height
    0.758                           // Mie preferred scattering direction
  );
  color = 1.0 - exp(-1.0 * color);
  FragColor = vec4(color, 1.0);
}
