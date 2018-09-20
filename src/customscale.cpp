#include "Noobhour.hpp"
#include "dsp/digital.hpp"
#include <vector>

// TODO
// - layout
// paging? different pages, back/forth, CV for index selection (interpolate between non-empty pages, lights to indicate?)


struct GreenBlueLight : GrayModuleLightWidget {
	GreenBlueLight() {
		addBaseColor(COLOR_GREEN);
		addBaseColor(COLOR_BLUE);
	}
};

template <typename BASE>
struct ToneLight : BASE {
  ToneLight() {
	this->box.size = mm2px(Vec(6.0f, 6.0f));
  }
};




struct CustomScale : Module {

  static const int NUM_OCTAVES = 5;
  static const int BASE_OCTAVE = 2;
  static const int NUM_TONES = NUM_OCTAVES * 12;
  
  enum InputIds {
	SIGNAL_INPUT,
	TONE_INPUT,
	TOGGLE_TRIGGER_INPUT,
	RESET_TRIGGER_INPUT,
	RANDOMIZE_TRIGGER_INPUT,
	BASE_INPUT,
	  
	NUM_INPUTS
  };
  
  enum OutputIds {
	OUT_OUTPUT,
	CHANGEGATE_OUTPUT,
	  
	NUM_OUTPUTS	  
  };

  enum ParamIds {
	ENUMS(TONE1_PARAM, NUM_TONES),
	RANGE_PARAM,
	P_PARAM,
	RANDOMIZE_BUTTON_PARAM,
	RESET_BUTTON_PARAM,
	BASE_PARAM,
	
	NUM_PARAMS
  };

  enum LightIds {
	ENUMS(TONE1_LIGHT, NUM_TONES * 2),
	ENUMS(RANDOMIZE_LIGHT, 2),	
	ENUMS(RESET_LIGHT, 2),
	
	NUM_LIGHTS
  };

  SchmittTrigger gateTrigger;
  SchmittTrigger randomizeTrigger;
  SchmittTrigger randomizeButtonTrigger;
  SchmittTrigger resetTrigger;
  SchmittTrigger resetButtonTrigger;
  SchmittTrigger paramTrigger[NUM_TONES];
  
  PulseGenerator changePulse;
  
  bool state[NUM_TONES];
  int lastOutput = NUM_TONES; // a value that wouldn't really be returned

  CustomScale() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
	onReset();	
  }
  
  void step() override;

  float getVOct(int toneIndex) const {
	int octave = toneIndex / 12;
	int tone = toneIndex % 12;
	return tone/12.f + octave - BASE_OCTAVE;
  }

  int getTone(float vOct) const {
	  return static_cast<int>(vOct * 12.f) + 12 * BASE_OCTAVE;
  }	

  void onReset() override {
	for (int i = 0; i < NUM_TONES; i++) {
	  state[i] = false;
	}
  }
  
  void onRandomize() override {
	randomizeTones(params[P_PARAM].value); 
  }

  void randomizeTones(float p) {
	for (int i = 0; i < NUM_TONES; i++) {
	  state[i] = (randomUniform() < p);
	}	
  }

  json_t *toJson() override {
	json_t *rootJ = json_object();

	json_t *statesJ = json_array();
	for (int i = 0; i < NUM_TONES; i++) {
	  json_t *stateJ = json_boolean(state[i]);
	  json_array_append_new(statesJ, stateJ);
	}
	json_object_set_new(rootJ, "states", statesJ);
	return rootJ;
  }
  
  void fromJson(json_t *rootJ) override {
	json_t *statesJ = json_object_get(rootJ, "states");
	if (statesJ) {
	  for (int i = 0; i < NUM_TONES; i++) {
		json_t *stateJ = json_array_get(statesJ, i);
		if (stateJ)
		  state[i] = json_boolean_value(stateJ);
	  }
	}
  }

};




void CustomScale::step() {
  // RESET
  if (inputs[RESET_TRIGGER_INPUT].active) {
	if (resetTrigger.process(rescale(inputs[RESET_TRIGGER_INPUT].value, 0.1f, 2.f, 0.f, 1.f))) {
	  onReset();
	}	
  }

  if (resetButtonTrigger.process(params[RESET_BUTTON_PARAM].value)) {
	onReset();
  }
  

  // RANDOMIZE
  if (inputs[RANDOMIZE_TRIGGER_INPUT].active) {
	if (randomizeTrigger.process(rescale(inputs[RANDOMIZE_TRIGGER_INPUT].value, 0.1f, 2.f, 0.f, 1.f))) {
	  randomizeTones(params[P_PARAM].value);
	}		
  }

  if (randomizeButtonTrigger.process(params[RANDOMIZE_BUTTON_PARAM].value)) {
	randomizeTones(params[P_PARAM].value);
  }

  // TOGGLE
  if (inputs[TONE_INPUT].active) {
	float gate = 0.0;
	if (inputs[TOGGLE_TRIGGER_INPUT].active)
	  gate = inputs[TOGGLE_TRIGGER_INPUT].value;
	if (gateTrigger.process(rescale(gate, 0.1f, 2.f, 0.f, 1.f))) {
	  int toneIndex = getTone(inputs[TONE_INPUT].value);
	  if (toneIndex >= 0 && toneIndex < NUM_TONES)
		state[toneIndex] ^= true;
	}
  }

  // OCTAVE RANGE
  int startTone = 0;
  int endTone = 0;
  int numCandidateTones = 0;
  if (params[RANGE_PARAM].value < 0.5f) {
	numCandidateTones = NUM_TONES;
	startTone = 0;
	endTone = NUM_TONES - 1;
  } else if (params[RANGE_PARAM].value < 1.5f) {
	numCandidateTones = NUM_TONES - 24;
	startTone = 12;
	endTone = NUM_TONES - 13;
  } else {
	numCandidateTones = NUM_TONES - 48;
	startTone = 24;
	endTone = NUM_TONES - 25;
  }

  // GATHER CANDIDATES
  std::vector<int> activeTones;
  activeTones.reserve(numCandidateTones);
  for (int i = 0; i < NUM_TONES; i++) {
	if (paramTrigger[i].process(params[i].value))
	  state[i] ^= true;
	if (state[i] && i >= startTone && i <= endTone) {
	  activeTones.push_back(i);
	} 	 
  }

  // FETCH BASE TONE
  float baseTone = params[BASE_PARAM].value;
  if (inputs[BASE_INPUT].active) {
	baseTone += inputs[BASE_INPUT].value / 10.f * 11.f;
  }
  int baseToneDiscrete = static_cast<int>(clamp(baseTone, 0.f, 11.f));

  // SELECT TONE  
  float output = 0;
  int selectedTone = -1;
  if (inputs[SIGNAL_INPUT].active && activeTones.size() > 0) {
	unsigned int selectedIndex = static_cast<int>(activeTones.size() * (clamp(inputs[SIGNAL_INPUT].value, -5.f, 5.f) + 5.f) / 10.f);
	if (selectedIndex == activeTones.size())
	  selectedIndex--;
	selectedTone = activeTones[selectedIndex];
	output = getVOct(selectedTone + baseToneDiscrete);
  }

  // DETECT TONE CHANGE
  if (output != lastOutput) {
	changePulse.trigger(0.001f);
	lastOutput = output;
  }  

  // LIGHTS
  for (int i = 0; i < NUM_TONES; i++) {
	float green = 0.f;
	float blue = 0.f;
	if (state[i]) {
	  if (i==selectedTone) {
		blue = 0.9f;
	  } else {
		if (i >= startTone && i <= endTone) {
		  green = 0.9f; // active tone but not selected
		} else {
		  green = 0.1f; // active but in inactive octave
		}		
	  }
	}
	lights[i * 2].setBrightness(green);
	lights[i * 2 + 1].setBrightness(blue);	  	  	
  }

  // OUTPUT
  outputs[OUT_OUTPUT].value = output;
  outputs[CHANGEGATE_OUTPUT].value = (changePulse.process(1.0f / engineGetSampleRate()) ? 10.0f : 0.0f);

}


struct CustomScaleWidget : ModuleWidget {
  CustomScaleWidget(CustomScale *module) : ModuleWidget(module) {
	
	setPanel(SVG::load(assetPlugin(plugin, "res/CustomScale.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

	// generate controls	
	static const int yStart = 21;
	static const int yRange = 40;
	static const int ySeparator = 5;
	static const int x = 11.5f;
	static const int x2 = 46.5f;

	static const float wKnob = 30.23437f;
	static const float wInput = 31.58030f;
	static const float wSwitch = 17.94267f;	
	static const float offsetKnob = -2.1; 
	static const float offsetSwitch = (wInput - wSwitch) / 2.0f - 1.5; // no idea why 1.5, not centered otherwise
	static const int offsetTL1005 = 4;

	addInput(Port::create<PJ301MPort>(Vec(x, yStart + 0 * yRange + 0 * ySeparator), Port::INPUT, module, CustomScale::SIGNAL_INPUT));
	addParam(ParamWidget::create<CKSSThree>(Vec(x2 + offsetSwitch + 4, yStart + 0 * yRange + 0 * ySeparator), module, CustomScale::RANGE_PARAM, 0.f, 2.f, 0.f));
	
	addOutput(Port::create<PJ301MPort>(Vec(x, yStart + 1 * yRange + 1 * ySeparator), Port::OUTPUT, module, CustomScale::OUT_OUTPUT));
	addOutput(Port::create<PJ301MPort>(Vec(x2, yStart + 1 * yRange + 1 * ySeparator), Port::OUTPUT, module, CustomScale::CHANGEGATE_OUTPUT));	

	addParam(ParamWidget::create<RoundBlackSnapKnob>(Vec(x + offsetKnob, yStart + 2 * yRange + 2 * ySeparator + offsetKnob), module, CustomScale::BASE_PARAM, 0.f, 11.f, 0.f));		
	addInput(Port::create<PJ301MPort>(Vec(x2, yStart + 2 * yRange + 2 * ySeparator), Port::INPUT, module, CustomScale::BASE_INPUT));

	
	addInput(Port::create<PJ301MPort>(Vec(x, 329 - (3 * yRange + 2 * ySeparator)), Port::INPUT, module, CustomScale::TONE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(x2, 329 - (3 * yRange + 2 * ySeparator)), Port::INPUT, module, CustomScale::TOGGLE_TRIGGER_INPUT)); 

	addInput(Port::create<PJ301MPort>(Vec(x, 329 - (2 * yRange + 1 * ySeparator)), Port::INPUT, module, CustomScale::RANDOMIZE_TRIGGER_INPUT));
	addParam(ParamWidget::create<TL1105>(Vec(x2 + offsetTL1005, 329 - (2 * yRange + 1 * ySeparator - offsetTL1005)), module, CustomScale::RANDOMIZE_BUTTON_PARAM, 0.0f, 1.0f, 0.0f));			
	addParam(ParamWidget::create<RoundBlackKnob>(Vec(x + offsetKnob, 329 - (1 * yRange + 1 * ySeparator - offsetKnob) - 2), module, CustomScale::P_PARAM, 0.f, 1.f, 0.5f));


	// 329
	addInput(Port::create<PJ301MPort>(Vec(x, 329), Port::INPUT, module, CustomScale::RESET_TRIGGER_INPUT));
	addParam(ParamWidget::create<TL1105>(Vec(x2 + offsetTL1005, 329  + offsetTL1005), module, CustomScale::RESET_BUTTON_PARAM, 0.0f, 1.0f, 0.0f));
	
	// generate lights
	float offsetX = mm2px(Vec(17.32, 18.915)).x - mm2px(Vec(16.57, 18.165)).x; // from Mutes
	float offsetY = mm2px(Vec(17.32, 18.915)).y - mm2px(Vec(16.57, 18.165)).y;	
	for (int octave=0; octave<CustomScale::NUM_OCTAVES; octave++) {
	  float x = 88 + octave * 27;
	  for (int tone=0; tone<12; tone++) {
		float y = -5 + 28 * (12 - tone);
		int index = octave * 12 + tone;
		
		addParam(ParamWidget::create<LEDBezel>(Vec(x, y), module, CustomScale::TONE1_PARAM + index, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<ToneLight<GreenBlueLight>>(Vec(x + offsetX, y + offsetY), module, CustomScale::TONE1_PARAM + index * 2));
	  }
	}
  };
};


Model *modelCustomScale = Model::create<CustomScale, CustomScaleWidget>("noobhour", "customscale", "CustomScale", RANDOM_TAG, DUAL_TAG);
