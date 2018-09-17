#include "Noobhour.hpp"
#include "dsp/digital.hpp"
#include <vector>

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
	  
	NUM_INPUTS
  };
  
  enum OutputIds {
	OUT_OUTPUT,
	  
	NUM_OUTPUTS	  
  };

  enum ParamIds {
	TONE1_PARAM,
	// ...
	RANGE_PARAM = NUM_TONES,
	P_PARAM = NUM_TONES + 1,
	
	NUM_PARAMS = NUM_TONES + 2
  };

  enum LightIds {
	TONE1_LIGHT,
	// ...
	
	NUM_LIGHTS = NUM_TONES * 2 
  };

  SchmittTrigger gateTrigger;
  SchmittTrigger randomizeTrigger;
  SchmittTrigger resetTrigger;
  SchmittTrigger paramTrigger[NUM_TONES];
  bool state[NUM_TONES];

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

  // RANDOMIZE
  if (inputs[RANDOMIZE_TRIGGER_INPUT].active) {
	if (randomizeTrigger.process(rescale(inputs[RANDOMIZE_TRIGGER_INPUT].value, 0.1f, 2.f, 0.f, 1.f))) {
	  randomizeTones(params[P_PARAM].value);
	}		
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

  // SELECT TONE
  float output = 0;
  int selectedTone = -1;
  if (inputs[SIGNAL_INPUT].active && activeTones.size() > 0) {
	unsigned int selectedIndex = static_cast<int>(activeTones.size() * (clamp(inputs[SIGNAL_INPUT].value, -5.f, 5.f)+5.f)/10.f);
	if (selectedIndex == activeTones.size())
	  selectedIndex--;
	selectedTone = activeTones[selectedIndex];
	output = getVOct(selectedTone);
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
}


struct CustomScaleWidget : ModuleWidget {
  CustomScaleWidget(CustomScale *module) : ModuleWidget(module) {
	
	setPanel(SVG::load(assetPlugin(plugin, "res/CustomScale.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

	// generate controls	
	int yStart = 21;
	int yRange = 37;
	int ySeparator = 19;
	int x = 10;

	float wKnob = 30.23437f;
	float wInput = 31.58030f;
	float wSwitch = 17.94267f;	
	float offsetKnob = wInput - wKnob;
	float offsetSwitch = (wInput - wSwitch) / 2.0f - 1.5; // no idea why 1.5, not centered otherwise

	addInput(Port::create<PJ301MPort>(Vec(x, yStart + 0 * yRange + 0 * ySeparator), Port::INPUT, module, CustomScale::SIGNAL_INPUT));
	addOutput(Port::create<PJ301MPort>(Vec(x, yStart + 1 * yRange + 0 * ySeparator), Port::OUTPUT, module, CustomScale::OUT_OUTPUT));
	addParam(ParamWidget::create<CKSSThree>(Vec(x + offsetSwitch, yStart + 2 * yRange + 0 * ySeparator), module, CustomScale::RANGE_PARAM, 0.f, 2.f, 0.f));
	
	addInput(Port::create<PJ301MPort>(Vec(x, yStart + 3 * yRange + 1 * ySeparator + 10), Port::INPUT, module, CustomScale::TONE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(x, yStart + 4 * yRange + 1 * ySeparator + 10), Port::INPUT, module, CustomScale::TOGGLE_TRIGGER_INPUT)); 

	addInput(Port::create<PJ301MPort>(Vec(x, yStart + 5 * yRange + 2 * ySeparator), Port::INPUT, module, CustomScale::RANDOMIZE_TRIGGER_INPUT));
	addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(x + offsetKnob, yStart + 6 * yRange + 2 * ySeparator), module, CustomScale::P_PARAM, 0.f, 1.f, 0.5f));	  	  
	
	addInput(Port::create<PJ301MPort>(Vec(x, 329), Port::INPUT, module, CustomScale::RESET_TRIGGER_INPUT));

	// generate lights
	float offsetX = mm2px(Vec(17.32, 18.915)).x - mm2px(Vec(16.57, 18.165)).x; // from Mutes
	float offsetY = mm2px(Vec(17.32, 18.915)).y - mm2px(Vec(16.57, 18.165)).y;	
	for (int octave=0; octave<CustomScale::NUM_OCTAVES; octave++) {
	  float x = 43 + octave * 27;
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
