#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

static juce::String hzText (float f) { return f >= 1000.f ? juce::String (f / 1000.f, 1) + " kHz" : juce::String ((int) f) + " Hz"; }
static juce::String pctText (float v) { return juce::String ((int) (v * 100.f)) + "%"; }

juce::AudioProcessorValueTreeState::ParameterLayout MikeProcessor::createLayout()
{
    using Fn = std::function<juce::String (float, int)>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    const Fn fns[10] = {
        [] (float v, int) { return juce::String (0.15 * std::pow (180.0, (double) v), 1) + " s"; },
        [] (float v, int) { return juce::String ((int) (30 + v * 3000)) + "/s"; },
        [] (float v, int) { return juce::String ((int) (2 + v * 100)) + " ms"; },
        [] (float v, int) { return pctText (v); },
        [] (float v, int) { return pctText (v); },
        [] (float v, int) { return juce::String ((int) (v * 200)) + " ms"; },
        [] (float v, int) { return pctText (v); },
        [] (float v, int) { return pctText (v); },
        [] (float v, int) { return hzText (20.f * std::pow (50.f, v)); },
        [] (float v, int) { return hzText (20000.f * std::pow (.05f, 1.f - v)); } };
    for (int i = 0; i < 10; ++i)
    {
        const float def = i < 8 ? kPresets[0].v[i] : (i == 8 ? kDefaultLowCut : kDefaultHighCut);
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kParamIds[i], 1 }, kParamNames[i],
               juce::NormalisableRange<float> (0.f, 1.f), def,
               juce::AudioParameterFloatAttributes().withStringFromValueFunction (fns[i])));
    }
    return l;
}

MikeProcessor::MikeProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("Mike IR builder"),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 10; ++i) P[i] = apvts.getRawParameterValue (kParamIds[i]);
    apvts.state.setProperty ("preset", 0, nullptr);
}

MikeProcessor::~MikeProcessor() { stopThread (5000); }

void MikeProcessor::loadPreset (int index)
{
    index = juce::jlimit (0, 7, index);
    const auto& p = kPresets[index];
    for (int i = 0; i < 10; ++i)
    {
        const float v = i < 8 ? p.v[i] : (i == 8 ? kDefaultLowCut : kDefaultHighCut);
        if (auto* prm = apvts.getParameter (kParamIds[i]))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (v);
            prm->endChangeGesture();
        }
    }
    apvts.state.setProperty ("preset", index, nullptr);
}

bool MikeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MikeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    sampleRateAtomic = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    conv.prepare (spec);
    delay.setMaximumDelayInSamples ((int) (0.25 * sampleRate));
    delay.prepare (spec);
    for (auto* f : { &damp, &lowCut, &highCut }) { f->prepare (spec); f->reset(); }
    damp.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    lowCut.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    highCut.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    dry.setSize (2, samplesPerBlock);
    const float mix = P[7]->load();
    dryG.reset (sampleRate, 0.03); dryG.setCurrentAndTargetValue (std::cos (mix * juce::MathConstants<float>::halfPi));
    wetG.reset (sampleRate, 0.03); wetG.setCurrentAndTargetValue (std::sin (mix * juce::MathConstants<float>::halfPi));
    preSm.reset (sampleRate, 0.05); preSm.setCurrentAndTargetValue (P[5]->load() * 0.2f * (float) sampleRate);
    irDirty = true;
    if (! isThreadRunning()) startThread();
}

void MikeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;

    // IR-shaping parameters changed? ask the builder thread for a new impulse response
    const float cur[5] = { P[0]->load(), P[1]->load(), P[2]->load(), P[3]->load(), P[6]->load() };
    for (int i = 0; i < 5; ++i)
        if (std::abs (cur[i] - lastIR[i]) > 1e-4f) { std::copy (cur, cur + 5, lastIR); irDirty = true; break; }

    // beat detection on the dry input (drives Mike's head bob)
    const float rms = 0.5f * (buffer.getRMSLevel (0, 0, n) + buffer.getRMSLevel (1, 0, n));
    sampleCounter += n;
    if (rms > 0.04f && rms > beatAvg * 1.5f && sampleCounter - lastBeatSample > (int64_t) (0.15 * sr))
    {
        lastBeatSample = sampleCounter;
        beatStrength = juce::jmin (1.3f, rms * 6.f);
        ++beatCount;
    }
    beatAvg += (1.f - std::exp (-(float) n / (0.2f * (float) sr))) * (rms - beatAvg);

    if (dry.getNumSamples() < n) dry.setSize (2, n, false, false, true);
    for (int c = 0; c < 2; ++c) dry.copyFrom (c, 0, buffer, c, 0, n);

    const float mix = P[7]->load();
    dryG.setTargetValue (std::cos (mix * juce::MathConstants<float>::halfPi));
    wetG.setTargetValue (std::sin (mix * juce::MathConstants<float>::halfPi));
    preSm.setTargetValue (P[5]->load() * 0.2f * (float) sr);
    const float nyq = 0.45f * (float) sr;
    damp.setCutoffFrequency (juce::jmin (nyq, 18000.f * std::pow (.04f, P[4]->load())));
    lowCut.setCutoffFrequency (20.f * std::pow (50.f, P[8]->load()));
    highCut.setCutoffFrequency (juce::jmin (nyq, 20000.f * std::pow (.05f, 1.f - P[9]->load())));

    // pre-delay
    for (int i = 0; i < n; ++i)
    {
        const float d = juce::jmax (1.f, preSm.getNextValue());
        for (int c = 0; c < 2; ++c)
        {
            delay.pushSample (c, buffer.getSample (c, i));
            buffer.setSample (c, i, delay.popSample (c, d, true));
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);
    conv.process (ctx);
    damp.process (ctx);
    lowCut.process (ctx);
    highCut.process (ctx);

    for (int i = 0; i < n; ++i)
    {
        const float dg = dryG.getNextValue(), wg = wetG.getNextValue();
        for (int c = 0; c < 2; ++c)
            buffer.setSample (c, i, dry.getSample (c, i) * dg + buffer.getSample (c, i) * wg);
    }
}

// ---- background IR builder -------------------------------------------------
void MikeProcessor::run()
{
    while (! threadShouldExit())
    {
        wait (40);
        if (! irDirty.exchange (false)) continue;
        wait (100);   // debounce knob drags
        const double rate = sampleRateAtomic.load();
        auto ir = buildIR (rate, P[0]->load(), P[1]->load(), P[2]->load(), P[3]->load(), P[6]->load());
        if (ir.getNumSamples() > 0)
            conv.loadImpulseResponse (std::move (ir), rate, juce::dsp::Convolution::Stereo::yes,
                                      juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
    }
}

juce::AudioBuffer<float> MikeProcessor::buildIR (double rate, float size, float dens, float grain, float scat, float width)
{
    const double decay = 0.15 * std::pow (180.0, (double) size);
    const double dur = std::min (decay * 1.35 + 0.05, 30.0);
    const int len = (int) (dur * rate);
    const int gs = std::max (8, (int) ((0.002 + grain * 0.1) * rate));
    int N = (int) ((30 + dens * 3000) * dur);
    N = std::max (40, std::min (N, (int) (25e6 / gs)));

    juce::AudioBuffer<float> buf (2, len);
    buf.clear();
    std::vector<float> win ((size_t) gs);
    for (int i = 0; i < gs; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) gs);

    juce::Random rnd;
    for (int c = 0; c < 2; ++c)
    {
        float* o = buf.getWritePointer (c);
        for (int g = 0; g < N; ++g)
        {
            if ((g & 255) == 0 && threadShouldExit()) return {};
            const float u = ((float) g + .5f) / (float) N, r = rnd.nextFloat();
            const float t = (u + (r - u) * scat) * (.12f + .88f * u + scat * .2f) / (1.f + .2f * scat);
            const int pos = juce::jlimit (0, len - gs - 1, (int) ((double) t * dur * rate));
            const float a = (float) std::exp (-6.9 * ((double) pos / rate) / decay) * (rnd.nextBool() ? -1.f : 1.f);
            float lp = 0.f; const float k = .15f + rnd.nextFloat() * .85f;
            for (int i = 0; i < gs; ++i)
            {
                lp += k * (rnd.nextFloat() * 2.f - 1.f - lp);
                o[pos + i] += lp * win[(size_t) i] * a;
            }
        }
    }

    // stereo width: blend channels toward mono
    float* l = buf.getWritePointer (0); float* r = buf.getWritePointer (1);
    const float w = .15f + width * .85f;
    for (int i = 0; i < len; ++i)
    {
        const float a = l[i], b = r[i];
        l[i] = a * (.5f + .5f * w) + b * (.5f - .5f * w);
        r[i] = b * (.5f + .5f * w) + a * (.5f - .5f * w);
    }
    // unit-energy normalise + 3 ms fade-in
    double e = 0; for (int i = 0; i < len; ++i) e += (double) l[i] * l[i];
    const float s = (float) (1.0 / std::sqrt (e + 1e-12));
    const int fi = std::max (1, (int) (rate * 0.003));
    for (int i = 0; i < len; ++i)
    {
        const float f = s * (i < fi ? (float) i / (float) fi : 1.f);
        l[i] *= f; r[i] *= f;
    }
    return buf;
}

juce::AudioProcessorEditor* MikeProcessor::createEditor() { return new MikeEditor (*this); }

void MikeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void MikeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MikeProcessor(); }
