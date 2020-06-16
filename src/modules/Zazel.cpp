/*
 * Copyright (c) 2020 Dave French <contact/dot/dave/dot/french3/at/googlemail/dot/com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "plugin.hpp"
#include "Zazel.h"
#include "easing.h"
#include "WidgetComposite.h"
#include "ctrl/SqMenuItem.h"
#include <atomic>
#include <assert.h>
#include "AudioMath.h"

using Comp = ZazelComp<WidgetComposite>;

struct RequestedParamId
{
    int moduleid = -1;
    int paramid = -1;
};

struct Zazel : Module
{
    std::shared_ptr<Comp> zazel;
    std::atomic<RequestedParamId> requestedParameter;
    ParamHandle paramHandle;
    std::atomic<bool> clearParam;
    Comp::Mode preLearnMode = Comp::Mode::PAUSED;
    float lastEnd;
    int endFrameCounter = 0;

    Zazel()
    {
        assert (std::atomic<RequestedParamId>{}.is_lock_free());
        config (Comp::NUM_PARAMS, Comp::NUM_INPUTS, Comp::NUM_OUTPUTS, Comp::NUM_LIGHTS);
        zazel = std::make_shared<Comp> (this);
        std::shared_ptr<IComposite> icomp = Comp::getDescription();
        SqHelper::setupParams (icomp, this);

        //init parameter handle
        paramHandle.color = nvgRGB (0xcd, 0xde, 0x87);
        APP->engine->addParamHandle (&paramHandle);
        clearParam.store (false);

        //init composite
        onSampleRateChange();
        zazel->init();
    }

    ~Zazel()
    {
        APP->engine->removeParamHandle (&paramHandle);
    }

    void onReset() override
    {
        RequestedParamId rpi;
        rpi.moduleid = -1;
        rpi.paramid = -1;
        requestedParameter.store (rpi);
    }

    json_t* dataToJson() override
    {
        json_t* rootJ = json_object();

        json_object_set_new (rootJ, "moduleId", json_integer (paramHandle.moduleId));
        json_object_set_new (rootJ, "parameterId", json_integer (paramHandle.paramId));

        return rootJ;
    }

    void dataFromJson (json_t* rootJ) override
    {
        json_t* moduleIdJ = json_object_get (rootJ, "moduleId");
        json_t* parameterIdJ = json_object_get (rootJ, "parameterId");
        if (! (moduleIdJ && parameterIdJ))
            return;
        RequestedParamId rpi;
        rpi.moduleid = json_integer_value (moduleIdJ);
        rpi.paramid = json_integer_value (parameterIdJ);
        requestedParameter.store (rpi);
    }

    void onSampleRateChange() override
    {
        float rate = SqHelper::engineGetSampleRate();
        zazel->setSampleRate (rate);
    }

    void updateParamHandle()
    {
        RequestedParamId rpi = requestedParameter.load();
        APP->engine->updateParamHandle (&paramHandle, rpi.moduleid, rpi.paramid, true);

        ParamQuantity* pq = paramHandle.module->paramQuantities[paramHandle.paramId];
        if (pq != nullptr)
        {
            lastEnd = pq->getScaledValue();
            zazel->setStartParamScaled (lastEnd);
            zazel->setEndParamScaled (lastEnd);
        }
    }

    void removeParam()
    {
        APP->engine->updateParamHandle (&paramHandle, -1, -1, true);
        clearParam.store (false);
        return;
    }

    void paramChange()
    {
        RequestedParamId rpi = requestedParameter.load();
        if (rpi.moduleid != -1)
        {
            //reset requested parameter so only updates on request.
            rpi.moduleid = -1;
            rpi.moduleid = -1;
            requestedParameter.store (rpi);

            //setup parameter learrning
            preLearnMode = zazel->mode;
            zazel->changePhase (Comp::Mode::LEARN_END);
            endFrameCounter = 0;
            lastEnd = 0.0f;
        }

        auto newParam = 0.0f;
        if (paramHandle.moduleId != -1 && paramHandle.module != nullptr)
        {
            ParamQuantity* pq = paramHandle.module->paramQuantities[paramHandle.paramId];
            if (pq != nullptr)
            {
                newParam = pq->getScaledValue();
            }
        }

        if (zazel->mode == Comp::Mode::LEARN_END && endFrameCounter > zazel->sampleRate)
        {
            zazel->changePhase (preLearnMode);
            endFrameCounter = 0;
        }
        else if (zazel->mode == Comp::Mode::LEARN_END
                 && (! sspo::AudioMath::areSame (lastEnd, newParam, 0.0001f)))
        {
            endFrameCounter = 0;
            lastEnd = newParam;
            zazel->setEndParamScaled (newParam);
        }
        else
        {
            endFrameCounter++;
        }
    }

    int getEasing()
    {
        return zazel->getCurrentEasing();
    }

    bool getOneShot()
    {
        return zazel->oneShot;
    }

    void process (const ProcessArgs& args) override
    {
        paramChange();
        zazel->step();
        if (! (paramHandle.moduleId == -1
               || paramHandle.module == nullptr
               || zazel->mode == Comp::Mode::LEARN_END))
        {
            ParamQuantity* pq = paramHandle.module->paramQuantities[paramHandle.paramId];
            if (pq != nullptr)
                pq->setScaledValue (zazel->out / 2.0f + 0.5);
        }
    }
};

/*****************************************************
User Interface
*****************************************************/

struct EasingWidget : Widget
{
    Zazel* module = nullptr;
    NVGcolor lineColor;
    Easings::EasingFactory ef;

    EasingWidget()
    {
        box.size = mm2px (Vec (14.142, 14.084));
        lineColor = nvgRGBA (0xf0, 0xf0, 0xf0, 0xff);
    }

    void setModule (Zazel* module)
    {
        this->module = module;
    }

    void draw (const DrawArgs& args) override
    {
        if (module == nullptr)
            return;
        const auto border = 14.142f * 0.1f; //bordersize in mm
        const auto width = 11.0f;
        const auto permm = width;
        auto easing = ef.getEasingVector().at (module->getEasing());

        nvgBeginPath (args.vg);
        nvgMoveTo (args.vg, mm2px (border), mm2px (border + width));
        for (auto i = 0.0f; i < 1.0f; i += 0.01f)
        {
            auto easingY = easing->easeInOut (i, 0.0f, 1.0f, 1.0f);
            nvgLineTo (args.vg,
                       mm2px (permm * i + border),
                       mm2px (border + width - width * easingY));
        }
        nvgStrokeColor (args.vg, lineColor);
        nvgStrokeWidth (args.vg, 1.5f);
        nvgStroke (args.vg);
    }
};

struct ParameterSelectWidget : Widget
{
    Zazel* module = nullptr;
    bool learning = false;

    std::shared_ptr<Font> font;
    NVGcolor txtColor;
    const int fontHeight = 12;

    ParameterSelectWidget()
    {
        box.size = mm2px (Vec (30.408, 14.084));
        font = APP->window->loadFont (asset::system ("res/fonts/ShareTechMono-Regular.ttf"));
        txtColor = nvgRGBA (0xf0, 0xf0, 0xf0, 0xff);
    }

    void setModule (Zazel* module)
    {
        this->module = module;
    }

    void onButton (const event::Button& e) override
    {
        e.stopPropagating();
        if (module == nullptr)
            return;

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT)
        {
            learning = true;
            module->removeParam();
            e.consume (this);
        }

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            RequestedParamId rpi;
            rpi.moduleid = -1;
            rpi.paramid = -1;
            module->requestedParameter.store (rpi);
            module->clearParam.store (true);
            module->removeParam();
            e.consume (this);
        }
    }

    //if the next object click on is a parameter, use it
    void onDeselect (const event::Deselect& e) override
    {
        if (module == nullptr)
            return;

        ParamWidget* touchedParam = APP->scene->rack->touchedParam;
        if (learning && touchedParam)
        {
            APP->scene->rack->touchedParam = nullptr;
            RequestedParamId rpi;
            rpi.moduleid = touchedParam->paramQuantity->module->id;
            rpi.paramid = touchedParam->paramQuantity->paramId;
            module->requestedParameter.store (rpi);
            module->updateParamHandle();
            learning = false;
        }
        else
            learning = false;
    }

    std::string getSelectedModuleName()
    {
        if (module == nullptr)
            return "";

        if (learning)
            return "learning";

        if (module->paramHandle.moduleId == -1)
            return "Module";
        else
        {
            ModuleWidget* mw = APP->scene->rack->getModule (module->paramHandle.moduleId);
            if (mw == nullptr)
                return "";
            return mw->model->name;
        }
    }

    std::string getSelectedParameterName()
    {
        if (module == nullptr)
            return "";

        if (learning)
            return "learning";

        if (module->paramHandle.moduleId == -1)
            return "Parameter";
        else
        {
            ModuleWidget* mw = APP->scene->rack->getModule (module->paramHandle.moduleId);
            if (mw == nullptr)
                return "";
            Module* m = mw->module;
            if (mw == nullptr)
                return "";
            auto paramId = module->paramHandle.paramId;
            if (paramId >= (int) m->params.size())
                return "";
            ParamQuantity* pq = m->paramQuantities[paramId];
            return pq->label;
        }
    }

    void draw (const DrawArgs& args) override
    {
        nvgFontSize (args.vg, fontHeight);
        nvgFontFaceId (args.vg, font->handle);
        //nvgTextLetterSpacing (args.vg, -2);
        nvgTextAlign (args.vg, NVG_ALIGN_LEFT);
        nvgFillColor (args.vg, txtColor);
        std::string txt;

        //module name text
        Vec c = Vec (5, 15);
        auto moduleTxt = getSelectedModuleName();
        moduleTxt.resize (14);
        nvgText (args.vg, c.x, c.y, moduleTxt.c_str(), NULL);
        auto parameterTxt = getSelectedParameterName();
        parameterTxt.resize (14);
        c = Vec (5, 35);
        nvgText (args.vg, c.x, c.y, parameterTxt.c_str(), NULL);
    }
};

struct ZazelButton : app::SvgSwitch
{
    ZazelButton()
    {
        momentary = true;
        addFrame (APP->window->loadSvg (asset::plugin (pluginInstance, "res/ZazelButton.svg")));
        addFrame (APP->window->loadSvg (asset::plugin (pluginInstance, "res/ZazelButton.svg")));
    }
};

struct ZazelTriggerButton : ZazelButton
{
    ZazelTriggerButton()
    {
        momentary = false;
    }
};

struct ZazelWidget : ModuleWidget
{
    ZazelWidget (Zazel* module)
    {
        setModule (module);
        std::shared_ptr<IComposite> icomp = Comp::getDescription();
        box.size = Vec (8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
        SqHelper::setPanel (this, "res/Zazel.svg");

        addChild (createWidget<ScrewSilver> (Vec (RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewSilver> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild (createWidget<ScrewSilver> (Vec (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild (createWidget<ScrewSilver> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam (SqHelper::createParamCentered<RoundLargeBlackKnob> (icomp, mm2px (Vec (48.161, 58.514)), module, Comp::START_PARAM));
        addParam (SqHelper::createParamCentered<RoundBlackKnob> (icomp, mm2px (Vec (28.925, 40.324)), module, Comp::EASING_ATTENUVERTER_PARAM));
        addParam (SqHelper::createParamCentered<RoundBlackKnob> (icomp, mm2px (Vec (28.925, 58.514)), module, Comp::START_ATTENUVERTER_PARAM));
        addParam (SqHelper::createParamCentered<RoundLargeBlackKnob> (icomp, mm2px (Vec (48.161, 40.324)), module, Comp::EASING_PARAM));
        addParam (SqHelper::createParamCentered<RoundBlackKnob> (icomp, mm2px (Vec (28.925, 76.704)), module, Comp::END_ATTENUVERTER_PARAM));
        addParam (SqHelper::createParamCentered<RoundLargeBlackKnob> (icomp, mm2px (Vec (48.161, 76.704)), module, Comp::END_PARAM));
        addParam (SqHelper::createParamCentered<RoundBlackKnob> (icomp, mm2px (Vec (28.925, 94.894)), module, Comp::DURATION_ATTENUVERTER_PARAM));
        addParam (SqHelper::createParamCentered<RoundLargeBlackKnob> (icomp, mm2px (Vec (48.161, 94.894)), module, Comp::DURATION_PARAM));
        addParam (SqHelper::createParamCentered<CKSS> (icomp, mm2px (Vec (5.05, 112.575)), module, Comp::ONESHOT_PARAM));
        addParam (SqHelper::createParamCentered<ZazelButton> (icomp, mm2px (Vec (16.93, 115.62)), module, Comp::SYNC_BUTTON_PARAM));
        addParam (SqHelper::createParamCentered<ZazelTriggerButton> (icomp, mm2px (Vec (28.814, 115.62)), module, Comp::TRIG_BUTTON_PARAM));
        addParam (SqHelper::createParamCentered<ZazelButton> (icomp, mm2px (Vec (40.697, 115.62)), module, Comp::PAUSE_BUTTON_PARAM));

        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (9.689, 40.324)), module, Comp::EASING_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (9.689, 58.514)), module, Comp::START_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (9.689, 76.704)), module, Comp::END_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (9.689, 94.894)), module, Comp::DURATION_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (40.697, 112.422)), module, Comp::STOP_CONT_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (16.93, 112.575)), module, Comp::CLOCK_INPUT));
        addInput (createInputCentered<PJ301MPort> (mm2px (Vec (28.814, 112.575)), module, Comp::START_CONT_INPUT));

        addOutput (createOutputCentered<PJ301MPort> (mm2px (Vec (52.581, 112.422)), module, Comp::MAIN_OUTPUT));

        addChild (createLightCentered<SmallLight<RedLight>> (mm2px (Vec (37.52, 108.25)), module, Comp::PAUSE_LIGHT));

        auto* paramSelectwidget = createWidget<ParameterSelectWidget> (mm2px (Vec (5.591, 14.19)));
        paramSelectwidget->setModule (module);
        addChild (paramSelectwidget);

        auto* easingWidget = createWidget<EasingWidget> (mm2px (Vec (40.315, 14.19)));
        easingWidget->setModule (module);

        addChild (easingWidget);
    }
};

Model* modelZazel = createModel<Zazel, ZazelWidget> ("Zazel");