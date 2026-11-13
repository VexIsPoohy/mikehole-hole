#include "PluginEditor.h"
#include "Presets.h"

static constexpr int W = 880, H = 640;

void MikeLAF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                float a0, float a1, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.f);
    const float d = juce::jmin (r.getWidth(), r.getHeight()), rad = d * .5f;
    auto c = r.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (.2f), c.x - rad * .3f, c.y - rad * .4f,
                                             juce::Colours::black.withAlpha (.35f), c.x + rad, c.y + rad, true));
    g.fillEllipse (c.x - rad, c.y - rad, d, d);
    g.setColour (acc);
    g.drawEllipse (c.x - rad, c.y - rad, d, d, 2.f);

    juce::Path arc;
    arc.addCentredArc (c.x, c.y, rad + 4.f, rad + 4.f, 0.f, a0, ang, true);
    g.setColour (acc.withAlpha (.55f));
    g.strokePath (arc, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (acc);
    g.drawLine (c.x + rad * .3f * std::sin (ang), c.y - rad * .3f * std::cos (ang),
                c.x + rad * .85f * std::sin (ang), c.y - rad * .85f * std::cos (ang), 3.5f);
}

MikeEditor::MikeEditor (MikeProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (W, H);
    cur = juce::jlimit (0, 7, (int) proc.apvts.state.getProperty ("preset", 0));

    for (int i = 0; i < 10; ++i)
    {
        auto& k = knobs[i];
        k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        addAndMakeVisible (k);
        att[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kParamIds[i], k);
        labels[i].setText (kParamNames[i], juce::dontSendNotification);
        labels[i].setJustificationType (juce::Justification::centred);
        labels[i].setFont (juce::Font (juce::FontOptions (13.f, juce::Font::bold)));
        addAndMakeVisible (labels[i]);
    }
    for (int i = 0; i < 8; ++i)
    {
        btn[i].setButtonText (kPresets[i].name);
        btn[i].onClick = [this, i] { proc.loadPreset (i); selectPreset (i); };
        addAndMakeVisible (btn[i]);
    }
    for (int i = 0; i < 4; ++i) cc[i] = tc[i] = juce::Colours::black;
    selectPreset (cur);
    for (int i = 0; i < 4; ++i) cc[i] = tc[i];
    applyColours();
    lastBeat = proc.beatCount.load();
    pickTarget();
    startTimerHz (60);
}

MikeEditor::~MikeEditor() { stopTimer(); setLookAndFeel (nullptr); }

float MikeEditor::param (const char* id) const
{
    const auto* value = proc.apvts.getRawParameterValue (id);
    jassert (value != nullptr);
    return value->load();
}

void MikeEditor::selectPreset (int i)
{
    cur = i;
    const auto& p = kPresets[i];
    tc[0] = juce::Colour (p.bg1); tc[1] = juce::Colour (p.bg2); tc[2] = juce::Colour (p.ink); tc[3] = juce::Colour (p.acc);
    for (int b = 0; b < 8; ++b) btn[b].setToggleState (b == i, juce::dontSendNotification);
}

void MikeEditor::applyColours()
{
    laf.acc = cc[3];
    for (int i = 0; i < 10; ++i)
    {
        knobs[i].setColour (juce::Slider::textBoxTextColourId, cc[2]);
        knobs[i].setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        knobs[i].setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        labels[i].setColour (juce::Label::textColourId, cc[2]);
    }
    for (auto& b : btn)
    {
        b.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::buttonOnColourId, cc[3]);
        b.setColour (juce::TextButton::textColourOffId, cc[2]);
        b.setColour (juce::TextButton::textColourOnId, cc[1]);
    }
}

void MikeEditor::pickTarget()
{
    tx = rnd.nextFloat() * (W - 100.f);
    ty = rnd.nextFloat() * (H - 130.f);
    trot = juce::degreesToRadians (rnd.nextFloat() * 60.f - 30.f);
    tscl = .8f + rnd.nextFloat() * .7f;
}

void MikeEditor::timerCallback()
{
    // theme cross-fade
    bool changed = false;
    for (int i = 0; i < 4; ++i)
        if (cc[i] != tc[i])
        {
            cc[i] = cc[i].interpolatedWith (tc[i], .12f);
            if (std::abs (cc[i].getRed() - tc[i].getRed()) < 3 && std::abs (cc[i].getGreen() - tc[i].getGreen()) < 3
                && std::abs (cc[i].getBlue() - tc[i].getBlue()) < 3) cc[i] = tc[i];
            changed = true;
        }
    if (changed) applyColours();

    // head bob on beat
    const int bc = proc.beatCount.load();
    if (bc != lastBeat) { lastBeat = bc; bobT = 0.f; bobAmp = proc.beatStrength.load(); dir = -dir; }
    if (bobT >= 0.f) { bobT += 1.f / 60.f / 0.3f; if (bobT >= 1.f) bobT = -1.f; }

    // random float-around
    if (++frame % 210 == 0) pickTarget();
    px += (tx - px) * .025f; py += (ty - py) * .025f;
    rot += (trot - rot) * .025f; scl += (tscl - scl) * .025f;

    if (cur == 6 && rnd.nextInt (240) == 0) flick = 1.f;   // backrooms fluorescent flicker
    flick *= .85f;
    repaint();
}

void MikeEditor::resized()
{
    const int bw = (W - 40 - 7 * 8) / 8;
    for (int i = 0; i < 8; ++i) btn[i].setBounds (20 + i * (bw + 8), 78, bw, 34);
    for (int i = 0; i < 10; ++i)
    {
        const int col = i % 5, row = i / 5;
        const int x = 20 + col * 168, y = 318 + row * 160;
        labels[i].setBounds (x, y, 168, 20);
        knobs[i].setBounds (x + 14, y + 20, 140, 130);
    }
}

void MikeEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (cc[0], W * .5f, 0.f, cc[1], W * .5f, H * 1.1f, true));
    g.fillAll();

    // per-preset background pattern
    switch (kPresets[cur].pattern)
    {
        case 0: g.setColour (juce::Colours::black.withAlpha (.12f));
                for (int x = 0; x < W; x += 40) g.fillRect (x, 0, 2, H); break;
        case 1: g.setColour (cc[2].withAlpha (.08f));
                for (int x = 0; x < W; x += 44) g.fillRect (x, 0, 1, H);
                for (int y = 0; y < H; y += 44) g.fillRect (0, y, W, 1); break;
        case 2: g.setColour (juce::Colours::white.withAlpha (.05f));
                for (int x = -H; x < W; x += 44) g.drawLine ((float) x, 0.f, (float) (x + H), (float) H, 8.f); break;
        case 3: g.setColour (cc[3].withAlpha (.1f));
                for (int x = 0; x < W; x += 120) g.fillRect (x, 0, 6, H); break;
        case 4: g.setColour (cc[3].withAlpha (.1f));
                for (int x = 0; x < W; x += 38) for (int y = 0; y < H; y += 38) g.fillEllipse ((float) x, (float) y, 4.f, 4.f); break;
        case 5: g.setColour (cc[3].withAlpha (.06f));
                for (int x = 0; x < W; x += 60) g.fillRect (x, 0, 2, H); break;
        case 6: g.setColour (cc[2].withAlpha (.13f));
                for (int x = 0; x < W; x += 32) g.fillRect (x, 0, 4, H); break;
        default: g.setColour (juce::Colours::black);
                g.fillEllipse (W * .5f - 90.f, H * .38f - 90.f, 180.f, 180.f);
                g.setColour (cc[3].withAlpha (.3f));
                g.drawEllipse (W * .5f - 94.f, H * .38f - 94.f, 188.f, 188.f, 4.f); break;
    }
    if (flick > .02f) { g.setColour (juce::Colours::black.withAlpha (flick * .35f)); g.fillAll(); }

    // title
    g.setColour (cc[3]);
    g.setFont (juce::Font (juce::FontOptions (40.f, juce::Font::bold)));
    g.drawText ("MIKE HOLES HOLE", 20, 14, 600, 56, juce::Justification::centredLeft);
    g.setColour (cc[2].withAlpha (.7f));
    g.setFont (juce::Font (juce::FontOptions (13.f)));
    g.drawText ("GRANULAR SPACE GENERATOR  /  CLOSET > BACKROOMS > VOID", 400, 24, 460, 36, juce::Justification::centredRight);

    // preset name + description
    const auto& p = kPresets[cur];
    g.setColour (cc[3]);
    g.setFont (juce::Font (juce::FontOptions (22.f, juce::Font::bold)));
    g.drawText (p.name, 20, 120, 240, 30, juce::Justification::centredLeft);
    g.setColour (cc[2].withAlpha (.75f));
    g.setFont (juce::Font (juce::FontOptions (14.f)));
    g.drawText (p.desc, 260, 120, 600, 30, juce::Justification::centredLeft);

    // grain-cloud visualiser (deterministic preview of the impulse response)
    auto vr = juce::Rectangle<float> (20.f, 154.f, W - 40.f, 150.f);
    g.setColour (juce::Colours::black.withAlpha (.28f));
    g.fillRoundedRectangle (vr, 10.f);
    const float size = param ("size"), scat = param ("scat"), grain = param ("grain");
    const double decay = .15 * std::pow (180.0, (double) size);
    const double dur = std::min (decay * 1.35 + .05, 30.0);
    juce::Random vrnd (42);
    const int N = 450;
    g.setColour (cc[3]);
    for (int i = 0; i < N; ++i)
    {
        const float u = ((float) i + .5f) / N, r = vrnd.nextFloat(), yy = vrnd.nextFloat();
        const float t = (u + (r - u) * scat) * (.12f + .88f * u + scat * .2f) / (1.f + .2f * scat);
        const float a = (float) std::exp (-6.9 * (double) t * dur / decay);
        g.setOpacity (juce::jlimit (.15f, 1.f, .25f + .75f * a));
        const float rad = 1.5f + grain * 5.f * (.3f + a);
        g.fillEllipse (vr.getX() + 12.f + t * (vr.getWidth() - 24.f) - rad, vr.getY() + 10.f + yy * (vr.getHeight() - 30.f) - rad, rad * 2, rad * 2);
    }
    g.setColour (cc[2].withAlpha (.6f));
    g.setFont (juce::Font (juce::FontOptions (11.f)));
    g.drawText ("0 s", (int) vr.getX() + 8, (int) vr.getBottom() - 18, 60, 14, juce::Justification::left);
    g.drawText (juce::String (decay, 1) + " s", (int) vr.getRight() - 68, (int) vr.getBottom() - 18, 60, 14, juce::Justification::right);

    // knob panel
    g.setColour (cc[2].withAlpha (.06f));
    g.fillRoundedRectangle (14.f, 314.f, W - 28.f, 316.f, 12.f);
}

void MikeEditor::paintOverChildren (juce::Graphics& g)
{
    if (! mike.isValid()) return;
    float dy = 0.f, brot = 0.f, bscl = 0.f;
    if (bobT >= 0.f)
    {
        const float env = bobT < .3f ? std::sin (bobT / .3f * juce::MathConstants<float>::halfPi)
                                     : std::cos ((bobT - .3f) / .7f * juce::MathConstants<float>::halfPi);
        dy = 18.f * bobAmp * env;
        brot = juce::degreesToRadians (9.f * bobAmp * dir) * env;
        bscl = .08f * bobAmp * env;
    }
    const float mw = 84.f, mh = 108.f;
    juce::Path rr; rr.addRoundedRectangle (0.f, 0.f, mw, mh, 12.f);
    g.saveState();
    g.addTransform (juce::AffineTransform::translation (-mw / 2, -mh / 2)
                        .scaled (scl * (1.f + bscl)).rotated (rot + brot)
                        .translated (px + mw / 2, py + mh / 2 + dy));
    g.setColour (cc[3].withAlpha (.25f)); g.strokePath (rr, juce::PathStrokeType (12.f));
    g.saveState();
    g.reduceClipRegion (rr, {});
    g.drawImage (mike, 0, 0, (int) mw, (int) mh, 0, 0, mike.getWidth(), mike.getHeight());
    g.restoreState();
    g.setColour (cc[3]); g.strokePath (rr, juce::PathStrokeType (3.f));
    g.restoreState();
}
