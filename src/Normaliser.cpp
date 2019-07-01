#include "Noobhour.hpp"
#include "plugin.hpp"


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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.2, 92.294)), module, Normaliser::MIN_PARAM));
		/*
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.2, 137.578)), module, Normaliser::MAX_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(19.2, 182.76)), module, Normaliser::ABS_PARAM));

		// addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.2, -66.891)), module, Normaliser::IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.2, 360.39)), module, Normaliser::CURRENTMIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.2, 400.286)), module, Normaliser::CURRENTMAX_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.2, 440.414)), module, Normaliser::OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.177, 236.876)), module, Normaliser::RESET_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.2, 269.295)), module, Normaliser::FREEZE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(19.2, 304.296)), module, Normaliser::POLY_LIGHT));
		*/
	}
};


Model *modelNormaliser = createModel<Normaliser, NormaliserWidget>("Normaliser");
