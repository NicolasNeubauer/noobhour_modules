#include "plugin.hpp"
#include "Noobhour.hpp"


struct Normaliser : Module {
  enum ParamIds {
	MIN_PARAM,
	MAX_PARAM,
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

  float minInput[PORT_MAX_CHANNELS];
  float maxInput[PORT_MAX_CHANNELS];

  

  Normaliser() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(MIN_PARAM, -10.f, 10.f, 0.f, "minimum output value", "V");
	configParam(MAX_PARAM, -10.f, 10.f, 10.f, "maximum output value", "V");
	configParam(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "reset");
	configParam(FREEZE_BUTTON_PARAM, 0.f, 1.f, 0.f, "freeze");
	configParam(POLY_BUTTON_PARAM, 0.f, 1.f, 0.f, "poly");
	initBoundaries();
  }

  void onReset() override {
	Module::onReset();
	initBoundaries();
  }

  void initBoundaries() {
	for (int i=0; i<PORT_MAX_CHANNELS; i++) {
	  minInput[i] = 11;
	  maxInput[i] = -11;
	}
  }

  json_t *dataToJson() override {
	json_t *rootJ = json_object();

	json_object_set_new(rootJ, "freeze", json_boolean(freeze));
	json_object_set_new(rootJ, "poly", json_boolean(poly));

	json_t *minJ = json_array();
	json_t *maxJ = json_array();	
	for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
	  json_array_append_new(minJ, json_real(minInput[c]));
	  json_array_append_new(maxJ, json_real(maxInput[c]));	  
	}
	json_object_set_new(rootJ, "min", minJ);
	json_object_set_new(rootJ, "max", maxJ);	
	return rootJ;
  }


  void dataFromJson(json_t *rootJ) override {
	freeze = json_boolean_value(json_object_get(rootJ, "freeze"));
	poly = json_boolean_value(json_object_get(rootJ, "poly"));	
	
	json_t *minJ = json_object_get(rootJ, "min");
	json_t *maxJ = json_object_get(rootJ, "max");	
	if (minJ && maxJ) {
	  for (int c = 0; c < PORT_MAX_CHANNELS; c++) {
		json_t *minValJ = json_array_get(minJ, c);
		if (minValJ)
		  minInput[c] = json_real_value(minValJ);
		json_t *maxValJ = json_array_get(maxJ, c);
		if (maxValJ)
		  maxInput[c] = json_real_value(maxValJ);
	  }
	}
  }

  void process(const ProcessArgs &args) override {
	// 1. UI events
	if (resetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue())) {
	  initBoundaries();
	}
	if (freezeButtonTrigger.process(params[FREEZE_BUTTON_PARAM].getValue())) {
	  freeze ^= true;
	}
	if (polyButtonTrigger.process(params[POLY_BUTTON_PARAM].getValue())) {
	  poly ^= true;
	  initBoundaries();
	}
	
	lights[FREEZE_LIGHT].setBrightness(freeze ? 1 : 0);
	lights[POLY_LIGHT].setBrightness(poly ? 1 : 0);

	// 2. update min/max
	int channels = inputs[IN_INPUT].getChannels();	
	if (!freeze) {
	  for (int c=0; c<channels; c++) {
		int boundaryChannel = poly ? c : 0; // use common boundaries unless poly is true
		float value = inputs[IN_INPUT].getVoltage(c);
		if (value > maxInput[boundaryChannel])
		  maxInput[boundaryChannel] = value;
		if (value < minInput[boundaryChannel])
		  minInput[boundaryChannel] = value;
	  }
	}

	// 3. scale
	outputs[OUT_OUTPUT].setChannels(channels);
	for (int c=0; c<channels; c++) {
		int boundaryChannel = poly ? c : 0;
		float currentMin = minInput[boundaryChannel];
		float currentMax = maxInput[boundaryChannel];
		float normalisedValue = 0;		
		if (currentMin != currentMax) 
		  normalisedValue = (inputs[IN_INPUT].getVoltage(c) - currentMin) / (currentMax - currentMin);
		float scaledValue = 0;
		float outputMin = params[MIN_PARAM].getValue();
	    float outputMax = params[MAX_PARAM].getValue();
		if (outputMax != outputMin) 
		  scaledValue += outputMin + normalisedValue * (outputMax - outputMin);
		// clamping for cases where freeze=True and input value outside of bounds
		outputs[OUT_OUTPUT].setVoltage(clamp(scaledValue, outputMin, outputMax), c);
	}

	if (poly) {
	  outputs[CURRENTMIN_OUTPUT].setChannels(channels);
	  outputs[CURRENTMAX_OUTPUT].setChannels(channels);
	  for (int c=0; c<channels; c++) {
		outputs[CURRENTMIN_OUTPUT].setVoltage(minInput[c], c);
		outputs[CURRENTMAX_OUTPUT].setVoltage(maxInput[c], c);
	  }
	} else {
	  outputs[CURRENTMIN_OUTPUT].setChannels(1);
	  outputs[CURRENTMIN_OUTPUT].setVoltage(minInput[0]);
	  outputs[CURRENTMAX_OUTPUT].setChannels(1);
	  outputs[CURRENTMAX_OUTPUT].setVoltage(maxInput[0]);
	}
	
  }
};


struct NormaliserWidget : ModuleWidget {
  NormaliserWidget(Normaliser *module) {
	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Normaliser.svg")));

	Vec resetPos = mm2px(Vec(10.16/2, 128.5-(64.875 + 4.009/ 2)));
	Vec freezePos = mm2px(Vec(10.16/2, 128.5-(55.504 + 4.009 / 2)));
	Vec polyPos = mm2px(Vec(10.16/2, 128.5-(46.177 + 4.009 / 2)));

	addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(96.54+7.937 / 2))), module, Normaliser::MIN_PARAM));
	addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.16/2, 128.5-(81.251 + 7.937 / 2))), module, Normaliser::MAX_PARAM));

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
