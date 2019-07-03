#include "plugin.hpp"
#include "Noobhour.hpp"


struct Normaliser : Module {
	enum ParamIds {
		MIN_PARAM,
		MAX_PARAM,
		ABS_PARAM,
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
		RESET_LIGHT,
		FREEZE_LIGHT,
		POLY_LIGHT,
		NUM_LIGHTS
	};

	Normaliser() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MIN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MAX_PARAM, 0.f, 1.f, 0.f, "");
		configParam(ABS_PARAM, 0.f, 1.f, 0.f, "");
	}

	void process(const ProcessArgs &args) override {
	}
};


struct NormaliserWidget : ModuleWidget {
	NormaliserWidget(Normaliser *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Normaliser.svg")));

		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(100.112+7.937/2))), module, Normaliser::MIN_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(88.13 + 7.937/ 2))), module, Normaliser::MAX_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(76.176 + 7.937/ 2))), module, Normaliser::ABS_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(112.000 + 7.858 / 2))), module, Normaliser::IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(28.424 + 8.026 / 2))), module, Normaliser::CURRENTMIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(17.868 + 8.026 / 2))), module, Normaliser::CURRENTMAX_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.16/2, 128.5-(7.317 + 7.761 / 2))), module, Normaliser::OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.16/2, 128.5-(63.822 + 4.009/ 2))), module, Normaliser::RESET_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.16/2, 128.5-(55.245 + 4.009 / 2))), module, Normaliser::FREEZE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(10.16/2, 128.5-(45.984 + 4.009 / 2))), module, Normaliser::POLY_LIGHT));
	}
};


Model *modelNormaliser = createModel<Normaliser, NormaliserWidget>("Normaliser");
