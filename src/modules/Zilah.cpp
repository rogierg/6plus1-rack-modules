#include "plugin.hpp"

//

// Midi cc is 7 bit, double is 14 bit
// Helper class for manipulation MSB LSB
// value is the 14 MSB, 0,0 for LSB
struct FourteenBit
{
    u_int16_t value;
    void setMsb (u_int8_t msb)
    {
        u_int16_t shiftedMsb = (u_int16_t) msb << 7U;
        value &= 0b0000000001111111U;
        value |= shiftedMsb;
    }

    void setLsb (u_int8_t lsb)
    {
        value &= 0b0011111110000000U;
        value |= lsb;
    }

    float getNormalised()
    {
        return static_cast<float> (value) / 0b0011111111111111;
    }

    FourteenBit()
    {
        value = 0;
    }
};

struct CcAggregator
{
    // MSB & LSB merged
    FourteenBit fourteenBit;

    virtual float get()
    {
        return fourteenBit.getNormalised();
    }
    virtual void setMsb (u_int8_t) = 0;
    virtual void setLsb (u_int8_t) = 0;
};

struct LsbOrMSbWithZeroingMidi10 : CcAggregator
{
    void setMsb (u_int8_t msb) override
    {
        fourteenBit.setMsb (msb);
        fourteenBit.setLsb (0);
    }

    void setLsb (u_int8_t lsb) override
    {
        fourteenBit.setLsb (lsb);
    }
};

struct LsbOrMSbWithoutZeroing : CcAggregator
{
    void setMsb (u_int8_t msb) override
    {
        fourteenBit.setMsb (msb);
    }

    void setLsb (u_int8_t lsb) override
    {
        fourteenBit.setLsb (lsb);
    }
};

struct MsbFirstWaitForLsb : CcAggregator
{
    u_int8_t msb = 0;
    bool isMsbSet = false;
    void setMsb (u_int8_t m) override
    {
        msb = m;
        isMsbSet = true;
    }
    void setLsb (u_int8_t l) override
    {
        if (isMsbSet)
        {
            fourteenBit.setMsb (msb);
            fourteenBit.setLsb (l);
            isMsbSet = false;
        }
        else
        {
            fourteenBit.setLsb (l);
        }
    }
};

struct Zilah : Module
{
    enum ParamIds
    {
        AGGREGATOR_PARAM,
        NUM_PARAMS
    };
    enum InputIds
    {
        NUM_INPUTS
    };
    enum OutputIds
    {
        MIDI_OUT_00_OUTPUT,
        MIDI_OUT_01_OUTPUT,
        MIDI_OUT_02_OUTPUT,
        MIDI_OUT_03_OUTPUT,
        MIDI_OUT_04_OUTPUT,
        MIDI_OUT_05_OUTPUT,
        MIDI_OUT_06_OUTPUT,
        MIDI_OUT_07_OUTPUT,
        MIDI_OUT_08_OUTPUT,
        MIDI_OUT_09_OUTPUT,
        MIDI_OUT_10_OUTPUT,
        MIDI_OUT_11_OUTPUT,
        MIDI_OUT_12_OUTPUT,
        MIDI_OUT_13_OUTPUT,
        MIDI_OUT_14_OUTPUT,
        MIDI_OUT_15_OUTPUT,
        MIDI_OUT_16_OUTPUT,
        MIDI_OUT_17_OUTPUT,
        MIDI_OUT_18_OUTPUT,
        MIDI_OUT_19_OUTPUT,
        MIDI_OUT_20_OUTPUT,
        MIDI_OUT_21_OUTPUT,
        MIDI_OUT_22_OUTPUT,
        MIDI_OUT_23_OUTPUT,
        MIDI_OUT_24_OUTPUT,
        MIDI_OUT_25_OUTPUT,
        MIDI_OUT_26_OUTPUT,
        MIDI_OUT_27_OUTPUT,
        MIDI_OUT_28_OUTPUT,
        MIDI_OUT_29_OUTPUT,
        MIDI_OUT_30_OUTPUT,
        MIDI_OUT_31_OUTPUT,

        NUM_OUTPUTS
    };
    enum LightIds
    {
        MSB_00_LIGHT,
        MSB_01_LIGHT,
        MSB_02_LIGHT,
        MSB_03_LIGHT,
        MSB_04_LIGHT,
        MSB_05_LIGHT,
        MSB_06_LIGHT,
        MSB_07_LIGHT,
        MSB_08_LIGHT,
        MSB_09_LIGHT,
        MSB_10_LIGHT,
        MSB_11_LIGHT,
        MSB_12_LIGHT,
        MSB_13_LIGHT,
        MSB_14_LIGHT,
        MSB_15_LIGHT,
        MSB_16_LIGHT,
        MSB_17_LIGHT,
        MSB_18_LIGHT,
        MSB_19_LIGHT,
        MSB_20_LIGHT,
        MSB_21_LIGHT,
        MSB_22_LIGHT,
        MSB_23_LIGHT,
        MSB_24_LIGHT,
        MSB_25_LIGHT,
        MSB_26_LIGHT,
        MSB_27_LIGHT,
        MSB_28_LIGHT,
        MSB_29_LIGHT,
        MSB_30_LIGHT,
        MSB_31_LIGHT,

        LSB_00_LIGHT,
        LSB_01_LIGHT,
        LSB_02_LIGHT,
        LSB_03_LIGHT,
        LSB_04_LIGHT,
        LSB_05_LIGHT,
        LSB_06_LIGHT,
        LSB_07_LIGHT,
        LSB_08_LIGHT,
        LSB_09_LIGHT,
        LSB_10_LIGHT,
        LSB_11_LIGHT,
        LSB_12_LIGHT,
        LSB_13_LIGHT,
        LSB_14_LIGHT,
        LSB_15_LIGHT,
        LSB_16_LIGHT,
        LSB_17_LIGHT,
        LSB_18_LIGHT,
        LSB_19_LIGHT,
        LSB_20_LIGHT,
        LSB_21_LIGHT,
        LSB_22_LIGHT,
        LSB_23_LIGHT,
        LSB_24_LIGHT,
        LSB_25_LIGHT,
        LSB_26_LIGHT,
        LSB_27_LIGHT,
        LSB_28_LIGHT,
        LSB_29_LIGHT,
        LSB_30_LIGHT,
        LSB_31_LIGHT,

        NUM_LIGHTS
    };

    midi::InputQueue midiInputQueue;
    std::vector<dsp::PulseGenerator> msbLedPulse{ 32 };
    std::vector<dsp::PulseGenerator> lsbLedPulse{ 32 };
    std::vector<LsbOrMSbWithZeroingMidi10> midi10Aggregator{ 32 };
    std::vector<LsbOrMSbWithoutZeroing> lsbOrMsbWithoutZeroing{ 32 };
    std::vector<MsbFirstWaitForLsb> msbFirstWaitForLsb{ 32 };

    enum Aggregators
    {
        Midi10,
        lsbMsbWithoutZeroing,
        msbFirstWaitForLsb_allLsbPass,
        NUM_AGGREGATORS
    };

    Zilah()
    {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam (AGGREGATOR_PARAM, 0, NUM_AGGREGATORS - 1, 0);
    }

    void process (const ProcessArgs& args) override
    {
        midi::Message msg;
        while (midiInputQueue.shift (&msg))
        {
            switch (msg.getStatus())
            {
                //note off
                case 0x8:
                {
                }
                break;

                    //note on
                case 0x9:
                {
                }
                break;
                    // cc
                case 0xb:
                {
                    auto cc = msg.getNote();
                    auto val = msg.getValue();
                    //14 bit cc are controllers MSB 0 - 31, LSB 32 - 63
                    if (cc < 32) // MSB
                    {
                        msbLedPulse[cc].trigger (0.5f);

                        //pass msb to all aggregators
                        midi10Aggregator[cc].setMsb (val);
                        lsbOrMsbWithoutZeroing[cc].setMsb (val);
                        msbFirstWaitForLsb[cc].setMsb (val);
                    }
                    else if (cc < 64) // LSB
                    {
                        lsbLedPulse[cc - 32].trigger();

                        //pass lsb to all aggregators
                        midi10Aggregator[cc - 32].setLsb (val);
                        lsbOrMsbWithoutZeroing[cc - 32].setLsb (val);
                        msbFirstWaitForLsb[cc - 32].setLsb (val);
                    }
                }

                break;
                default:
                {
                }
            }
        }
        //outputLeds
        for (auto i = 0; i < 32; ++i)
        {
            lights[LSB_00_LIGHT + i].setBrightness (lsbLedPulse[i].process (args.sampleTime));
            lights[MSB_00_LIGHT + i].setBrightness (msbLedPulse[i].process (args.sampleTime));
        }

        //output aggregators
        //TODO case on selected aggregators
        for (auto i = 0; i < 32; ++i)
        {
            switch ((int) params[AGGREGATOR_PARAM].getValue())
            {
                case Midi10:
                    outputs[MIDI_OUT_00_OUTPUT + i].setVoltage (midi10Aggregator[i].get() * 10.0f);
                    break;
                case lsbMsbWithoutZeroing:
                    outputs[MIDI_OUT_00_OUTPUT + i].setVoltage (lsbOrMsbWithoutZeroing[i].get() * 10.0f);
                    break;
                case msbFirstWaitForLsb_allLsbPass:
                    outputs[MIDI_OUT_00_OUTPUT + i].setVoltage (msbFirstWaitForLsb[i].get() * 10.0f);
                    break;
                default:
                    break;
            }
        }
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();
        json_object_set_new (rootJ, "midiInput", midiInputQueue.toJson());
        return rootJ;
    }

    void dataFromJson (json_t* rootJ) override
    {
        json_t* midiInputJ = json_object_get (rootJ, "midiInput");
        if (midiInputJ)
            midiInputQueue.fromJson (midiInputJ);
    }
};

//****************************************************************8
//*********** UI
//*****************************************************************

struct AggregatorMenuItem : MenuItem
{
    int aggregator = 0;
    Zilah* module;

    void onAction (const event::Action& e) override
    {
        module->params[Zilah::AGGREGATOR_PARAM].setValue (aggregator);
    }
};

struct Midi_cc_14Widget : ModuleWidget
{
    Midi_cc_14Widget (Zilah* module)
    {
        setModule (module);
        setPanel (APP->window->loadSvg (asset::plugin (pluginInstance, "res/Zilah.svg")));

        addChild (createWidget<ScrewSilver> (Vec (RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewSilver> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewSilver> (Vec (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild (createWidget<ScrewSilver> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (79.823, 54.724)), module, Zilah::MIDI_OUT_06_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (56.756, 54.725)), module, Zilah::MIDI_OUT_04_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (68.289, 54.725)), module, Zilah::MIDI_OUT_05_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (10.621, 54.872)), module, Zilah::MIDI_OUT_00_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (91.356, 54.977)), module, Zilah::MIDI_OUT_07_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (22.155, 54.997)), module, Zilah::MIDI_OUT_01_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (33.688, 54.998)), module, Zilah::MIDI_OUT_02_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (45.222, 54.998)), module, Zilah::MIDI_OUT_03_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (56.756, 72.378)), module, Zilah::MIDI_OUT_12_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (68.289, 72.378)), module, Zilah::MIDI_OUT_13_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (79.823, 72.378)), module, Zilah::MIDI_OUT_14_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (10.621, 72.525)), module, Zilah::MIDI_OUT_08_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (91.356, 72.63)), module, Zilah::MIDI_OUT_15_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (22.155, 72.651)), module, Zilah::MIDI_OUT_09_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (33.688, 72.651)), module, Zilah::MIDI_OUT_10_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (45.222, 72.652)), module, Zilah::MIDI_OUT_11_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (56.756, 90.031)), module, Zilah::MIDI_OUT_20_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (68.289, 90.031)), module, Zilah::MIDI_OUT_21_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (79.823, 90.031)), module, Zilah::MIDI_OUT_22_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (10.621, 90.179)), module, Zilah::MIDI_OUT_16_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (91.356, 90.283)), module, Zilah::MIDI_OUT_23_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (22.155, 90.304)), module, Zilah::MIDI_OUT_17_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (33.688, 90.304)), module, Zilah::MIDI_OUT_18_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (45.222, 90.305)), module, Zilah::MIDI_OUT_19_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (56.756, 107.301)), module, Zilah::MIDI_OUT_28_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (68.289, 107.301)), module, Zilah::MIDI_OUT_29_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (79.823, 107.301)), module, Zilah::MIDI_OUT_30_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (10.621, 107.449)), module, Zilah::MIDI_OUT_24_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (91.356, 107.553)), module, Zilah::MIDI_OUT_31_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (22.155, 107.574)), module, Zilah::MIDI_OUT_25_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (33.688, 107.574)), module, Zilah::MIDI_OUT_26_OUTPUT));
        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (45.222, 107.575)), module, Zilah::MIDI_OUT_27_OUTPUT));

        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (53.578, 50.547)), module, Zilah::MSB_04_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (59.933, 50.547)), module, Zilah::LSB_04_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (65.111, 50.547)), module, Zilah::MSB_05_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (71.467, 50.547)), module, Zilah::LSB_05_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (76.645, 50.547)), module, Zilah::MSB_06_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (83.0, 50.547)), module, Zilah::LSB_06_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (7.443, 50.694)), module, Zilah::MSB_00_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (13.799, 50.694)), module, Zilah::LSB_00_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (88.178, 50.799)), module, Zilah::MSB_07_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (94.534, 50.799)), module, Zilah::LSB_07_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (25.333, 50.819)), module, Zilah::LSB_01_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (18.977, 50.82)), module, Zilah::MSB_01_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (30.511, 50.82)), module, Zilah::MSB_02_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (36.866, 50.82)), module, Zilah::LSB_02_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (42.044, 50.82)), module, Zilah::MSB_03_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (48.4, 50.82)), module, Zilah::LSB_03_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (53.578, 68.2)), module, Zilah::MSB_12_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (59.933, 68.2)), module, Zilah::LSB_12_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (65.111, 68.2)), module, Zilah::MSB_13_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (71.467, 68.2)), module, Zilah::LSB_13_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (76.645, 68.2)), module, Zilah::MSB_14_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (83.0, 68.2)), module, Zilah::LSB_14_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (7.443, 68.348)), module, Zilah::MSB_08_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (13.799, 68.348)), module, Zilah::LSB_08_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (88.178, 68.452)), module, Zilah::MSB_15_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (94.534, 68.452)), module, Zilah::LSB_15_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (18.977, 68.473)), module, Zilah::MSB_09_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (25.333, 68.473)), module, Zilah::LSB_09_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (30.511, 68.473)), module, Zilah::MSB_10_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (36.866, 68.473)), module, Zilah::LSB_10_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (42.044, 68.474)), module, Zilah::MSB_11_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (48.4, 68.474)), module, Zilah::LSB_11_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (53.578, 85.853)), module, Zilah::MSB_20_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (59.933, 85.853)), module, Zilah::LSB_20_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (65.111, 85.853)), module, Zilah::MSB_21_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (71.467, 85.853)), module, Zilah::LSB_21_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (76.645, 85.853)), module, Zilah::MSB_22_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (83.0, 85.853)), module, Zilah::LSB_22_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (7.443, 86.001)), module, Zilah::MSB_16_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (13.799, 86.001)), module, Zilah::LSB_16_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (88.178, 86.105)), module, Zilah::MSB_23_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (94.534, 86.105)), module, Zilah::LSB_23_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (18.977, 86.126)), module, Zilah::MSB_17_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (25.333, 86.126)), module, Zilah::LSB_17_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (30.511, 86.127)), module, Zilah::MSB_18_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (36.866, 86.127)), module, Zilah::LSB_18_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (42.044, 86.127)), module, Zilah::MSB_19_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (48.4, 86.127)), module, Zilah::LSB_19_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (53.578, 103.123)), module, Zilah::MSB_28_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (59.933, 103.123)), module, Zilah::LSB_28_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (65.111, 103.123)), module, Zilah::MSB_29_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (71.467, 103.123)), module, Zilah::LSB_29_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (76.645, 103.123)), module, Zilah::MSB_30_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (83.0, 103.123)), module, Zilah::LSB_30_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (7.443, 103.271)), module, Zilah::MSB_24_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (13.799, 103.271)), module, Zilah::LSB_24_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (94.534, 103.375)), module, Zilah::LSB_31_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (88.178, 103.376)), module, Zilah::MSB_31_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (18.977, 103.396)), module, Zilah::MSB_25_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (25.333, 103.396)), module, Zilah::LSB_25_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (30.511, 103.397)), module, Zilah::MSB_26_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (36.866, 103.397)), module, Zilah::LSB_26_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (42.044, 103.397)), module, Zilah::MSB_27_LIGHT));
        addChild (createLightCentered<SmallLight<GreenLight>> (mm2px (Vec (48.4, 103.397)), module, Zilah::LSB_27_LIGHT));

        if (module)
        {
            auto* midiAInWidget = createWidget<MidiWidget> (mm2px (Vec (30, 14.211)));
            midiAInWidget->box.size = mm2px (Vec (40, 25));
            midiAInWidget->setMidiPort (&module->midiInputQueue);
            addChild (midiAInWidget);
        }
    }

    void appendContextMenu (Menu* menu)
    {
        auto* module = dynamic_cast<Zilah*> (this->module);
        menu->addChild (new MenuEntry);

        auto* midi10aggregator = new AggregatorMenuItem();
        midi10aggregator->aggregator = Zilah::Aggregators::Midi10;
        midi10aggregator->text = "Midi 1.0";
        midi10aggregator->module = (Zilah*) module;
        midi10aggregator->rightText = CHECKMARK (((Zilah*) module)->params[Zilah::AGGREGATOR_PARAM].getValue()
                                                 == Zilah::Aggregators::Midi10);
        menu->addChild (midi10aggregator);

        auto* noZeroaggregator = new AggregatorMenuItem();
        noZeroaggregator->aggregator = Zilah::Aggregators::lsbMsbWithoutZeroing;
        noZeroaggregator->text = "No Zeroing, No Waiting";
        noZeroaggregator->module = (Zilah*) module;
        noZeroaggregator->rightText = CHECKMARK (((Zilah*) module)->params[Zilah::AGGREGATOR_PARAM].getValue()
                                                 == Zilah::Aggregators::lsbMsbWithoutZeroing);
        menu->addChild (noZeroaggregator);

        auto* msbWaitForLsb = new AggregatorMenuItem();
        msbWaitForLsb->aggregator = Zilah::Aggregators::msbFirstWaitForLsb_allLsbPass;
        msbWaitForLsb->text = "MSB waits for LSB";
        msbWaitForLsb->module = (Zilah*) module;
        msbWaitForLsb->rightText = CHECKMARK (((Zilah*) module)->params[Zilah::AGGREGATOR_PARAM].getValue()
                                              == Zilah::Aggregators::msbFirstWaitForLsb_allLsbPass);
        menu->addChild (msbWaitForLsb);
    }
};

Model* modelZilah = createModel<Zilah, Midi_cc_14Widget> ("Zilah");