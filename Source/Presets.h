#pragma once

#include <cstdint>

inline constexpr const char* kParamIds[10] {
    "size", "density", "grain", "scat", "damp",
    "predelay", "width", "mix", "lowcut", "highcut"
};

inline constexpr const char* kParamNames[10] {
    "SIZE", "DENSITY", "GRAIN", "SCATTER", "DAMPING",
    "PRE-DELAY", "WIDTH", "MIX", "LOW CUT", "HIGH CUT"
};

struct MikePreset
{
    const char* name;
    const char* desc;
    std::uint32_t bg1, bg2, ink, acc;
    int pattern;
    float v[8];
};

inline constexpr float kDefaultLowCut = 0.f;
inline constexpr float kDefaultHighCut = 1.f;

inline constexpr MikePreset kPresets[8] {
    { "CLOSET", "A short, intimate room with soft reflections.", 0xff241a18, 0xff49342a, 0xfff1d8b8, 0xfff2a66f, 0, { .18f, .22f, .25f, .18f, .35f, .05f, .4f, .25f } },
    { "HALLWAY", "Long reflections stretch into the distance.", 0xff141e2b, 0xff243c50, 0xffd5e5eb, 0xff66c6d8, 0, { .42f, .30f, .20f, .35f, .28f, .12f, .55f, .32f } },
    { "POOLROOM", "Cool, wide reflections with a glassy tail.", 0xff102b31, 0xff1a5360, 0xffd1f1e8, 0xff70e0ca, 1, { .58f, .42f, .12f, .22f, .58f, .08f, .78f, .38f } },
    { "LIMINAL", "A soft cloud suspended in a strange space.", 0xff29203b, 0xff54406c, 0xffeee0fa, 0xffd29cff, 2, { .70f, .34f, .44f, .52f, .30f, .18f, .72f, .42f } },
    { "DREAM", "Diffuse grains blur the edges of the room.", 0xff302a21, 0xff655339, 0xffffeec7, 0xffffcf70, 3, { .76f, .54f, .62f, .68f, .42f, .15f, .62f, .45f } },
    { "TUNNEL", "A dense, narrow trail of repeating echoes.", 0xff211d27, 0xff4b354b, 0xfff0d7eb, 0xfff078bc, 4, { .64f, .66f, .18f, .42f, .52f, .10f, .35f, .40f } },
    { "BACKROOMS", "A flickering fluorescent hum in endless rooms.", 0xff24291d, 0xff51563a, 0xffe6ebc6, 0xffc7e66a, 6, { .86f, .48f, .32f, .58f, .66f, .22f, .68f, .50f } },
    { "VOID", "An immense, dark tail opening into nothing.", 0xff090b12, 0xff171629, 0xffd7d2f2, 0xffa58cff, 7, { 1.f, .72f, .48f, .30f, .22f, .30f, .9f, .58f } }
};
