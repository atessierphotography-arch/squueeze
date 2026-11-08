#include "PluginEditor.h"

namespace
{
    constexpr int W = 1043, H = 374;

    const juce::Colour panelTop    { 0xff2f2f2f };
    const juce::Colour panelBottom { 0xff1a1a1a };
    const juce::Colour silk        { 0xffe6e6e6 };
    const juce::Colour frameLine   { 0xffd2d2d2 };

    const juce::Colour ledRed    { 0xffff3b30 };
    const juce::Colour ledYellow { 0xffffd60a };
    const juce::Colour ledGreen  { 0xff3ddc4a };
    const juce::Colour ledOrange { 0xffff9a1f };
    const juce::Colour ledCyan   { 0xff19d3ff };

    const float knobX[4] = { 180.0f, 378.0f, 577.0f, 775.0f };
    const float knobY = 288.0f;

    juce::Colour panelAt (float y) { return panelTop.interpolatedWith (panelBottom, y / (float) H); }

    juce::Font makeFont (float size, bool bold = true, bool italic = false, float hscale = 0.86f)
    {
        int style = (bold ? juce::Font::bold : 0) | (italic ? juce::Font::italic : 0);
        return juce::Font (juce::FontOptions (size, style)).withHorizontalScale (hscale);
    }

    void T (juce::Graphics& g, const juce::String& s, float cx, float cy, float size,
            bool bold = true, float hs = 0.86f, juce::Colour c = silk)
    {
        g.setColour (c);
        g.setFont (makeFont (size, bold, false, hs));
        g.drawText (s, juce::Rectangle<float> (cx - 120.0f, cy - size, 240.0f, size * 2.0f), juce::Justification::centred);
    }

    void erase (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (panelAt (r.getCentreY()));
        g.fillRect (r);
    }

    void drawLed (juce::Graphics& g, float x, float y, juce::Colour c, bool on, float r = 6.0f)
    {
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.55f), x, y, c.withAlpha (0.0f), x + r * 2.4f, y, true));
            g.fillEllipse (x - r * 2.4f, y - r * 2.4f, r * 4.8f, r * 4.8f);
        }
        g.setColour (juce::Colours::black);
        g.fillEllipse (x - r - 1.0f, y - r - 1.0f, r * 2.0f + 2.0f, r * 2.0f + 2.0f);
        g.setGradientFill (juce::ColourGradient (on ? c.brighter (0.6f) : c.withMultipliedBrightness (0.5f),
                                                 x - r * 0.3f, y - r * 0.4f,
                                                 on ? c.darker (0.2f) : c.withMultipliedBrightness (0.2f),
                                                 x + r * 0.7f, y + r * 0.8f, true));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
        if (on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.fillEllipse (x - r * 0.45f, y - r * 0.55f, r * 0.5f, r * 0.4f);
        }
    }

    juce::Path hpIcon (float x, float y)
    {
        juce::Path p;
        p.startNewSubPath (x, y + 9.0f);
        p.quadraticTo (x + 10.0f, y + 8.0f, x + 15.0f, y - 2.0f);
        p.quadraticTo (x + 18.0f, y - 7.0f, x + 24.0f, y - 7.0f);
        p.lineTo (x + 34.0f, y - 7.0f);
        return p;
    }

    juce::Path bumpIcon (float x, float y)
    {
        juce::Path p;
        p.startNewSubPath (x, y + 7.0f);
        p.lineTo (x + 6.0f, y + 7.0f);
        p.quadraticTo (x + 17.0f, y - 18.0f, x + 28.0f, y + 7.0f);
        p.lineTo (x + 34.0f, y + 7.0f);
        return p;
    }
}

void SilverKnobLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& slider)
{
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float sn = std::sin (angle), cs = -std::cos (angle);

    // Petit bouton noir (MIX)
    if (slider.getProperties().contains ("black"))
    {
        const float r = 26.0f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - r + 2.0f, c.y - r + 4.0f, r * 2.0f, r * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4d), c.x - r * 0.6f, c.y - r * 0.8f,
                                                 juce::Colour (0xff050506), c.x + r * 0.6f, c.y + r * 0.9f, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.fillEllipse (c.x - r * 0.6f, c.y - r * 0.85f, r * 1.0f, r * 0.5f);
        g.setColour (silk);
        g.drawLine (c.x + sn * 4.0f, c.y + cs * 4.0f, c.x + sn * r * 0.85f, c.y + cs * r * 0.85f, 3.0f);
        return;
    }

    // Graduations et chiffres 0..10 sur l'anneau extérieur
    g.setColour (silk);
    for (int i = 0; i <= 50; ++i)
    {
        const float a = startAngle + (float) i / 50.0f * (endAngle - startAngle);
        const float len = (i % 5 == 0) ? 7.0f : 4.0f;
        g.drawLine (c.x + std::sin (a) * 56.0f, c.y - std::cos (a) * 56.0f,
                    c.x + std::sin (a) * (56.0f + len), c.y - std::cos (a) * (56.0f + len), 0.9f);
    }
    g.setFont (makeFont (11.0f));
    for (int n = 0; n <= 10; ++n)
    {
        const float a = startAngle + (float) n / 10.0f * (endAngle - startAngle);
        const float px = c.x + std::sin (a) * 69.0f, py = c.y - std::cos (a) * 69.0f;
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (a, px, py));
        g.drawText (juce::String (n), juce::Rectangle<float> (px - 9.0f, py - 7.0f, 18.0f, 14.0f), juce::Justification::centred);
    }

    // Ombre
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - 50.0f + 2.0f, c.y - 50.0f + 4.0f, 100.0f, 100.0f);

    // Corps argenté moleté
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f2f2), c.x - 30.0f, c.y - 40.0f,
                                             juce::Colour (0xff767676), c.x + 35.0f, c.y + 45.0f, true));
    g.fillEllipse (c.x - 50.0f, c.y - 50.0f, 100.0f, 100.0f);
    for (int i = 0; i < 90; ++i)
    {
        const float a = (float) i / 90.0f * juce::MathConstants<float>::twoPi;
        g.setColour (i % 2 ? juce::Colours::black.withAlpha (0.28f) : juce::Colours::white.withAlpha (0.35f));
        g.drawLine (c.x + std::sin (a) * 40.0f, c.y - std::cos (a) * 40.0f,
                    c.x + std::sin (a) * 50.0f, c.y - std::cos (a) * 50.0f, 1.2f);
    }
    g.setColour (juce::Colour (0xff4a4a4a));
    g.drawEllipse (c.x - 50.0f, c.y - 50.0f, 100.0f, 100.0f, 1.5f);

    // Dessus plat gris avec anneaux
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d0d0), c.x - 25.0f, c.y - 30.0f,
                                             juce::Colour (0xff8c8c8c), c.x + 25.0f, c.y + 30.0f, false));
    g.fillEllipse (c.x - 36.0f, c.y - 36.0f, 72.0f, 72.0f);
    g.setColour (juce::Colour (0xff555555));
    g.drawEllipse (c.x - 36.0f, c.y - 36.0f, 72.0f, 72.0f, 1.2f);
    g.setColour (juce::Colour (0xffa4a4a4));
    g.fillEllipse (c.x - 26.0f, c.y - 26.0f, 52.0f, 52.0f);
    g.setColour (juce::Colour (0xff6a6a6a));
    g.drawEllipse (c.x - 26.0f, c.y - 26.0f, 52.0f, 52.0f, 1.0f);

    // Repère gravé
    g.setColour (juce::Colour (0xff2b2b2b));
    g.drawLine (c.x + sn * 30.0f, c.y + cs * 30.0f, c.x + sn * 48.0f, c.y + cs * 48.0f, 2.6f);
}

void SilverButton::paintButton (juce::Graphics& g, bool, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0d0d0d));
    g.fillRoundedRectangle (r, 9.0f);
    auto in = r.reduced (3.0f);
    g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xff9a9a9a) : juce::Colour (0xffe6e6e6), in.getX(), in.getY(),
                                             down ? juce::Colour (0xffd0d0d0) : juce::Colour (0xff7c7c7c), in.getX(), in.getBottom(), false));
    g.fillRoundedRectangle (in, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawRoundedRectangle (in.reduced (0.5f), 7.0f, 1.0f);
    g.setColour (juce::Colour (0xff5a5a5a).withAlpha (0.6f));
    g.drawRoundedRectangle (in.reduced (5.0f, 4.0f), 5.0f, 1.0f);
}

void RockerButton::paintButton (juce::Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colour (0xff5a5a5a));
    g.drawRoundedRectangle (r, 6.0f, 1.5f);

    auto face = r.reduced (5.0f);
    g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff4a4a4a) : juce::Colour (0xff171717), face.getX(), face.getY(),
                                             on ? juce::Colour (0xff141414) : juce::Colour (0xff4a4a4a), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle (face, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawLine (face.getX() + 3.0f, on ? face.getY() + 2.0f : face.getBottom() - 2.0f,
                face.getRight() - 3.0f, on ? face.getY() + 2.0f : face.getBottom() - 2.0f, 1.2f);
}

void HrSwitch::paintButton (juce::Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (silk);
    g.drawEllipse (r, 1.6f);
    g.drawLine (r.getX() + 3.0f, r.getCentreY(), r.getRight() - 3.0f, r.getCentreY(), 1.6f);
    if (getToggleState())
    {
        g.setColour (ledOrange);
        g.fillEllipse (r.reduced (5.0f));
    }
}

SqueezeEditor::SqueezeEditor (SqueezeProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto setup = [this] (juce::Slider& s)
    {
        s.setLookAndFeel (&laf);
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setRotaryParameters (juce::degreesToRadians (220.0f), juce::degreesToRadians (500.0f), true);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };
    for (auto* s : { &inputKnob, &attackKnob, &releaseKnob, &outputKnob, &mixKnob }) setup (*s);
    mixKnob.getProperties().set ("black", true);

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    inA = std::make_unique<SA> (processor.apvts, ParamID::input,   inputKnob);
    atA = std::make_unique<SA> (processor.apvts, ParamID::attack,  attackKnob);
    reA = std::make_unique<SA> (processor.apvts, ParamID::release, releaseKnob);
    ouA = std::make_unique<SA> (processor.apvts, ParamID::output,  outputKnob);
    miA = std::make_unique<SA> (processor.apvts, ParamID::mix,     mixKnob);

    for (auto* b : { (juce::Button*) &bypassBtn, (juce::Button*) &ratioBtn, (juce::Button*) &detectorBtn,
                     (juce::Button*) &audioBtn, (juce::Button*) &powerBtn, (juce::Button*) &hrBtn })
        addAndMakeVisible (*b);

    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    bypassBtn.setClickingTogglesState (true);
    powerBtn.setClickingTogglesState (true);
    hrBtn.setClickingTogglesState (true);
    byA = std::make_unique<BA> (processor.apvts, ParamID::bypass, bypassBtn);
    poA = std::make_unique<BA> (processor.apvts, ParamID::power,  powerBtn);
    hrA = std::make_unique<BA> (processor.apvts, ParamID::hr,     hrBtn);

    ratioBtn.onClick = [this] { cycleParam (ParamID::ratio, 9); };
    audioBtn.onClick = [this] { cycleParam (ParamID::audio, 4); };
    detectorBtn.onClick = [this]
    {
        // HP -> HP + bosse -> bosse -> rien
        const bool hp = getParam (ParamID::detHP) > 0.5f, bump = getParam (ParamID::detBump) > 0.5f;
        if (! hp && ! bump)    { setParam (ParamID::detHP, 1.0f); setParam (ParamID::detBump, 0.0f); }
        else if (hp && ! bump) { setParam (ParamID::detHP, 1.0f); setParam (ParamID::detBump, 1.0f); }
        else if (hp && bump)   { setParam (ParamID::detHP, 0.0f); setParam (ParamID::detBump, 1.0f); }
        else                   { setParam (ParamID::detHP, 0.0f); setParam (ParamID::detBump, 0.0f); }
    };

    setSize (W, H);
    startTimerHz (30);
}

SqueezeEditor::~SqueezeEditor()
{
    stopTimer();
    for (auto* s : { &inputKnob, &attackKnob, &releaseKnob, &outputKnob, &mixKnob }) s->setLookAndFeel (nullptr);
}

float SqueezeEditor::getParam (const char* id) const
{
    return processor.apvts.getRawParameterValue (id)->load();
}

void SqueezeEditor::setParam (const char* id, float realValue)
{
    if (auto* prm = processor.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
        prm->endChangeGesture();
    }
}

void SqueezeEditor::cycleParam (const char* id, int count)
{
    setParam (id, (float) ((juce::roundToInt (getParam (id)) + 1) % count));
}

void SqueezeEditor::resized()
{
    inputKnob.setBounds   (juce::Rectangle<int> (150, 150).withCentre ({ (int) knobX[0], (int) knobY }));
    attackKnob.setBounds  (juce::Rectangle<int> (150, 150).withCentre ({ (int) knobX[1], (int) knobY }));
    releaseKnob.setBounds (juce::Rectangle<int> (150, 150).withCentre ({ (int) knobX[2], (int) knobY }));
    outputKnob.setBounds  (juce::Rectangle<int> (150, 150).withCentre ({ (int) knobX[3], (int) knobY }));
    mixKnob.setBounds     (juce::Rectangle<int> (70, 70).withCentre ({ 938, 289 }));

    bypassBtn.setBounds   (313, 72, 68, 28);
    ratioBtn.setBounds    (591, 112, 60, 28);
    detectorBtn.setBounds (688, 112, 65, 28);
    audioBtn.setBounds    (785, 112, 66, 28);
    powerBtn.setBounds    (903, 60, 55, 62);
    hrBtn.setBounds       (49, 277, 20, 20);

    buildBackground();
}

void SqueezeEditor::buildBackground()
{
    background = juce::Image (juce::Image::RGB, W, H, true);
    juce::Graphics g (background);

    // Face avant : tôle noire texturée
    g.setGradientFill (juce::ColourGradient (panelTop, 0.0f, 0.0f, panelBottom, 0.0f, (float) H, false));
    g.fillAll();
    juce::Random rnd (5);
    for (int i = 0; i < 26000; ++i)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.02f + 0.06f * rnd.nextFloat()));
        g.fillRect (rnd.nextInt (W), rnd.nextInt (H), 1, 1);
    }

    const juce::PathStrokeType stroke (2.6f);
    g.setColour (frameLine);

    // Cadre du haut
    juce::Path top;
    top.addRoundedRectangle (8.0f, 14.0f, 1025.0f, 153.0f, 26.0f);
    g.strokePath (top, stroke);
    erase (g, { 143.0f, 9.0f, 520.0f, 11.0f });     // trou pour les chiffres du GR
    erase (g, { 882.0f, 9.0f, 92.0f, 11.0f });      // trou pour POWER
    erase (g, { 176.0f, 160.0f, 700.0f, 14.0f });   // le bas est remplacé par le cadre RATIO / boutons

    // Cadre RATIO
    juce::Path ratioFrame;
    ratioFrame.addRoundedRectangle (193.0f, 126.0f, 482.0f, 41.0f, 14.0f);
    g.setColour (frameLine);
    g.strokePath (ratioFrame, stroke);
    erase (g, { 214.0f, 120.0f, 440.0f, 13.0f });
    erase (g, { 340.0f, 160.0f, 88.0f, 14.0f });

    // Cadre du bas
    juce::Path bottom;
    bottom.addRoundedRectangle (8.0f, 202.0f, 1025.0f, 155.0f, 26.0f);
    g.setColour (frameLine);
    g.strokePath (bottom, stroke);
    const float gaps[5][2] = { { 117.0f, 40.0f }, { 309.0f, 46.0f }, { 505.0f, 50.0f }, { 712.0f, 50.0f }, { 911.0f, 24.0f } };
    for (auto& gp : gaps) erase (g, { gp[0] - gp[1], 196.0f, gp[1] * 2.0f, 13.0f });
    g.setColour (frameLine);
    for (float tx : { 180.0f, 378.0f, 577.0f, 775.0f, 935.0f })
        g.drawLine (tx, 202.0f, tx, 220.0f, 2.2f);

    // Titres des sections
    T (g, "INPUT", 117, 203, 15);
    T (g, "ATTACK", 309, 203, 15);
    T (g, "RELEASE", 505, 203, 15);
    T (g, "OUTPUT", 712, 203, 15);
    T (g, "MIX", 911, 203, 15);
    T (g, "POWER", 931, 14, 15);
    T (g, "RATIO", 384, 166, 18);

    // Compteur de réduction de gain
    const char* grNums[16] = { "26", "23", "20", "17", "14", "12", "10", "9", "8", "7", "6", "5", "4", "3", "2", "1" };
    for (int i = 0; i < 16; ++i) T (g, grNums[i], 173.0f + 31.0f * (float) i, 15, 14);
    T (g, "GAIN REDUCTION", 412, 52, 16);

    // Logo (nom et marque propres)
    g.setColour (silk);
    g.setFont (makeFont (60.0f, true, true, 0.82f));
    g.drawText ("Squeeze", juce::Rectangle<float> (100.0f, 52.0f, 230.0f, 66.0f), juce::Justification::centredLeft);
    {
        juce::Path d;
        d.addPolygon ({ 78.0f, 100.0f }, 4, 26.0f, juce::MathConstants<float>::pi / 4.0f);
        g.strokePath (d, juce::PathStrokeType (2.4f));
        T (g, "MW", 78, 100, 13);
    }

    // BY PASS
    T (g, "BY", 421, 85, 15);
    T (g, "PASS", 470, 85, 15);

    // Icône sinus tronqué
    juce::Path sine;
    for (int i = 0; i <= 60; ++i)
    {
        const float xx = 503.0f + (float) i;
        const float yy = 76.0f - juce::jlimit (-6.0f, 8.0f, 9.0f * std::sin ((float) i / 60.0f * juce::MathConstants<float>::twoPi));
        if (i == 0) sine.startNewSubPath (xx, yy); else sine.lineTo (xx, yy);
    }
    g.setColour (silk);
    g.strokePath (sine, juce::PathStrokeType (2.0f));

    T (g, "REDLINE", 619, 60, 12);
    T (g, juce::String ("1% THD"), 611, 86, 12);

    // Rapports de compression
    const char* ratioNames[8] = { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "20:1", "NUKE" };
    const float rx[8] = { 220, 267, 315, 363, 409, 457, 504, 551 };
    for (int i = 0; i < 8; ++i) T (g, ratioNames[i], rx[i], 127, 13, true, 0.86f, i == 7 ? silk : silk);
    T (g, "Opto", 470, 158, 12);

    // Boutons : légendes
    T (g, "RATIO", 621, 154, 16);
    T (g, "DETECTOR", 720, 156, 15);
    T (g, "AUDIO", 818, 156, 16);

    // Détecteur
    g.setColour (silk);
    g.strokePath (hpIcon (688, 38), juce::PathStrokeType (2.0f));
    T (g, "HP", 724, 35, 13);
    g.strokePath (bumpIcon (688, 62), juce::PathStrokeType (2.0f));
    T (g, "Link", 711, 86, 13);

    // Audio
    g.strokePath (hpIcon (783, 38), juce::PathStrokeType (2.0f));
    T (g, "HP", 820, 35, 13);
    T (g, "Dist 2", 809, 61, 13);
    T (g, "Dist 3", 809, 86, 13);

    // Marque à droite du POWER
    T (g, "MIXWAVES", 997, 86, 10);
    T (g, "SQUEEZE", 997, 99, 10);

    // HR
    T (g, "HR", 59, 262, 13);

    // Légendes sous les boutons
    T (g, "Opto(10)", 467, 342, 11);
    T (g, "Opto(0)", 661, 342, 11);
    T (g, "Dry", 914, 322, 11);
    T (g, "Comp", 965, 322, 11);

    // Têtes de vis aux angles
    g.setColour (juce::Colour (0xff101010));
    for (auto p : { juce::Point<float> (14.0f, 12.0f), { (float) W - 14.0f, 12.0f }, { 14.0f, (float) H - 12.0f }, { (float) W - 14.0f, (float) H - 12.0f } })
        g.fillEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
}

void SqueezeEditor::mouseDown (const juce::MouseEvent& e)
{
    struct Hit { float x, y; const char* id; int bit; };
    static const Hit hits[] = {
        { 748.0f, 35.0f, ParamID::detHP,   -1 }, { 748.0f, 60.0f, ParamID::detBump, -1 }, { 748.0f, 85.0f, ParamID::link, -1 },
        { 845.0f, 35.0f, ParamID::audioHP, -1 }, { 845.0f, 60.0f, ParamID::audio, 1 },    { 845.0f, 86.0f, ParamID::audio, 2 }
    };
    for (auto& h : hits)
    {
        if (e.position.getDistanceFrom ({ h.x, h.y }) > 12.0f) continue;
        if (h.bit < 0) setParam (h.id, getParam (h.id) > 0.5f ? 0.0f : 1.0f);
        else           setParam (h.id, (float) (juce::roundToInt (getParam (h.id)) ^ h.bit));
        return;
    }
}

void SqueezeEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    const bool on = getParam (ParamID::power) > 0.5f;
    const bool active = on && getParam (ParamID::bypass) < 0.5f;
    const float gr = active ? processor.grMeter.load() : 0.0f;
    const float pk = active ? processor.outPeak.load() : 0.0f;

    // LED de réduction de gain : rouge (26-12), jaune (10-7), vert (6-1)
    static const float vals[16] = { 26, 23, 20, 17, 14, 12, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
    for (int i = 0; i < 16; ++i)
    {
        const auto c = i < 6 ? ledRed : (i < 10 ? ledYellow : ledGreen);
        drawLed (g, 173.0f + 31.0f * (float) i, 35.0f, c, on && vals[i] <= gr);
    }

    // Rapports : 1:1 2:1 3:1 4:1 6:1 10:1 20:1 NUKE (+ Opto)
    const int ratio = juce::roundToInt (getParam (ParamID::ratio));
    const int lit = ratio <= 5 ? ratio : (ratio == 6 ? 5 : (ratio == 7 ? 6 : 7));
    const juce::Colour rc[8] = { ledRed, ledGreen, ledGreen, ledGreen, ledYellow, ledOrange, ledRed, ledCyan };
    const float rx[8] = { 220, 267, 315, 363, 409, 457, 504, 551 };
    for (int i = 0; i < 8; ++i) drawLed (g, rx[i], 143.0f, rc[i], on && i == lit, 5.5f);
    drawLed (g, 446.0f, 158.0f, ledOrange, on && ratio == 6, 5.0f);

    drawLed (g, 438.0f, 85.0f, ledRed, getParam (ParamID::bypass) > 0.5f, 5.5f);
    drawLed (g, 577.0f, 60.0f, ledRed, on && pk > 0.7f, 5.5f);
    drawLed (g, 577.0f, 86.0f, ledYellow, on && pk > 0.25f, 5.5f);

    drawLed (g, 748.0f, 35.0f, ledGreen,  on && getParam (ParamID::detHP) > 0.5f);
    drawLed (g, 748.0f, 60.0f, ledYellow, on && getParam (ParamID::detBump) > 0.5f);
    drawLed (g, 748.0f, 85.0f, ledRed,    on && getParam (ParamID::link) > 0.5f);

    const int audio = juce::roundToInt (getParam (ParamID::audio));
    drawLed (g, 845.0f, 35.0f, ledGreen,  on && getParam (ParamID::audioHP) > 0.5f);
    drawLed (g, 845.0f, 60.0f, ledOrange, on && (audio & 1) != 0);
    drawLed (g, 845.0f, 86.0f, ledRed,    on && (audio & 2) != 0);
}

void SqueezeEditor::timerCallback()
{
    repaint();
}
