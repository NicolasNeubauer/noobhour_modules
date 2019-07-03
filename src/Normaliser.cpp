#include "plugin.hpp"
#include "Noobhour.hpp"


struct Normaliser : Module {
  enum ParamIds {
	MIN_PARAM,
	MAX_PARAM,
	ABS_PARAM,
	RESET_BUTTON_PARAM,
	FREEZE_BUTTON_PARAM,
	POLY_BUTTON_PARAM,		
	NUM_PARAMS
  };
  enum InputIds {
	IN_INPUT,
	NUM_INPUTS
  };
  enum OutputIds {
	CURRENTMIN_OUTPUT,
	CURRENTMAX_OUTPUT,
	OUT_OUTPUT,
	NUM_OUTPUTS
  };
  enum LightIds {
	// RESET_LIGHT,
	FREEZE_LIGHT,
	POLY_LIGHT,
	NUM_LIGHTS
  };
  
  dsp::SchmittTrigger resetButtonTrigger;
  dsp::SchmittTrigger freezeButtonTrigger;
  dsp::SchmittTrigger polyButtonTrigger;

  bool freeze = false;
  bool poly = false;

  

  Normaliser() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(MIN_PARAM, -10.f, 10.f, 0.f, "minimum output value", "V");
	configParam(MAX_PARAM, -10.f, 10.f, 10.f, "maximum output value", "V");
	configParam(ABS_PARAM, -10.f, 10.f, 0.f, "absolute offset", "V");
	configParam(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "reset");
	configParam(FREEZE_BUTTON_PARAM, 0.f, 1.f, 0.f, "freeze");
	configParam(POLY_BUTTON_PARAM, 0.f, 1.f, 0.f, "poly");		
  }

  void onReset() override {
	
  }

  void process(const ProcessArgs &args) override {
	if (resetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue())) {
	  onReset();
	}
	if (freezeButtonTrigger.process(params[FREEZE_BUTTON_PARAM].getValue())) {
	  freeze ^= true;
	}
	if (polyButtonTrigger.process(params[POLY_BUTTON_PARAM].getValue())) {
	  poly ^= true;
	}
	
	
	lights[FREEZE_LIGHT].setBrightness(freeze ? 1 : 0);
	lights[POLY_LIGHT].setBrightness(poly ? 1 : 0);
  }
};


struct NormaliserWidget : ModuleWidget {
  NormaliserWidget(Normaliser *module) {
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Normaliser.svg")));

	Vec resetPos = mm2px(Vec(10.16/2, 128.5-(64.4 + 4.009/ 2)));
	Vec freezePos = mm2px(Vec(10.16/2, 128.5-(55.245 + 4.009 / 2)));
	Vec polyPos = mm2px(Vec(10.16/2, 128.5-(45.684 + 4.009 / 2)));

	addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(100.112+7.937/2))), module, Normaliser::MIN_PARAM));
	addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(88.13 + 7.937/ 2))), module, Normaliser::MAX_PARAM));
	addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(76.176 + 7.937/ 2))), module, Normaliser::ABS_PARAM));
	addParam(createParamCentered<TL1105>(resetPos, module, Normaliser::RESET_BUTTON_PARAM));
	addParam(createParamCentered<TL1105>(freezePos, module, Normaliser::FREEZE_BUTTON_PARAM));
	addParam(createParamCentered<TL1105>(polyPos, module, Normaliser::POLY_BUTTON_PARAM)); 				

	addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(112.000 + 7.858 / 2))), module, Normaliser::IN_INPUT));

	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(28.424 + 8.026 / 2))), module, Normaliser::CURRENTMIN_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(17.868 + 8.026 / 2))), module, Normaliser::CURRENTMAX_OUTPUT));
	addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(7.317 + 7.761 / 2))), module, Normaliser::OUT_OUTPUT));

	//addChild(createLightCentered<MediumLight<RedLight>>(resetPos, module, Normaliser::RESET_LIGHT));
	addChild(createLightCentered<MediumLight<GreenLight>>(freezePos, module, Normaliser::FREEZE_LIGHT));
	addChild(createLightCentered<MediumLight<GreenLight>>(polyPos, module, Normaliser::POLY_LIGHT));
  }
};


Model *modelNormaliser = createModel<Normaliser, NormaliserWidget>("Normaliser");
