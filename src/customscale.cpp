#include "Noobhour.hpp"
#include "dsp/digital.hpp"
#include <vector>

// TODO: Randomize, p

struct CustomScale : Module {
  
  enum InputIds {
	SIGNAL_INPUT,
	TONE_INPUT,
	TOGGLE_TRIGGER_INPUT,
	  
	NUM_INPUTS
  };
  
  enum OutputIds {
	OUT_OUTPUT,
	  
	NUM_OUTPUTS	  
  };

  /*
  enum LightIds {
	NUM_LIGHTS
  };
  */

  /*
  enum ParamIds {
	TONE1_PARAM
  };
  */

  static const int NUM_OCTAVES = 5;
  static const int BASE_OCTAVE = 2; // which column contains the 440Hz A?
  static const int NUM_PARAMS = NUM_OCTAVES * 12;
  static const int NUM_LIGHTS = NUM_PARAMS;

  double RATIO = 1.0594630943592953; // pow(2.0, 1.0/12.0) - ratio between one semi-tone and the next

  SchmittTrigger gateTrigger;
  SchmittTrigger paramTrigger[NUM_PARAMS];
  bool state[NUM_PARAMS];

  CustomScale() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
  
  void step() override;

  float getFrequency(int paramId) const {
	int octave = paramId / 12;
	int tone = paramId % 12;

	return octave - BASE_OCTAVE + pow(RATIO, tone);
	// int distanceFromA = (octave - BASE_OCTAVE) * 12 + tone - 9;
	// return (octave * 12 + tone) * RATIO;
	//return 440.f * pow(RATIO, distanceFromA);
  }


	void onReset() override {
		for (int i = 0; i < NUM_PARAMS; i++) {
			state[i] = false;
		}
	}
	void onRandomize() override {
		for (int i = 0; i < NUM_PARAMS; i++) {
			state[i] = (randomUniform() < 0.5f);
		}
	}

	json_t *toJson() override {
		json_t *rootJ = json_object();
		// states
		json_t *statesJ = json_array();
		for (int i = 0; i < NUM_PARAMS; i++) {
			json_t *stateJ = json_boolean(state[i]);
			json_array_append_new(statesJ, stateJ);
		}
		json_object_set_new(rootJ, "states", statesJ);
		return rootJ;
	}
	void fromJson(json_t *rootJ) override {
		// states
		json_t *statesJ = json_object_get(rootJ, "states");
		if (statesJ) {
			for (int i = 0; i < NUM_PARAMS; i++) {
				json_t *stateJ = json_array_get(statesJ, i);
				if (stateJ)
					state[i] = json_boolean_value(stateJ);
			}
		}
	}

  

};

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0f, 6.0f));
	}
};

/*
float CustomScale::getFreqency(int paramId) {
  int octave = paramId / 12;
  int tone = paramId % 12;
  int distanceFromA = (octave - BASE_OCTAVE) * 12 + tone - 10;
  return 440.f * pow(ratio, distanceFromA);
}
*/


void CustomScale::step() {
  
  if (inputs[TONE_INPUT].active) {
	float gate = 0.0;
	if (inputs[TOGGLE_TRIGGER_INPUT].active)
	  gate = inputs[TOGGLE_TRIGGER_INPUT].value;
	if (gateTrigger.process(rescale(gate, 0.1f, 2.f, 0.f, 1.f))) {
	  float v = inputs[TONE_INPUT].value;
	  int paramIndex = static_cast<int>(NUM_PARAMS * (v+5.f)/10.f);
	  if (paramIndex == NUM_PARAMS)
		paramIndex--;
	  // TODO: toggle param paramIndex;
	}
  }

  std::vector<int> activeParams;
  activeParams.reserve(NUM_PARAMS);
  for (int i=0; i < NUM_PARAMS; i++) {
	if (paramTrigger[i].process(params[i].value))
	  state[i] ^= true;
	if (state[i]) {
	  activeParams.push_back(i);
	  lights[i].setBrightness(0.9f);
	} else {
	  lights[i].setBrightness(0.0f);	  
	}	 
  }

  if (!inputs[SIGNAL_INPUT].active || activeParams.size() == 0) {
	outputs[OUT_OUTPUT].value = 0;
	return;
  }  

  unsigned int selectedIndex = static_cast<int>(activeParams.size() * (clamp(inputs[SIGNAL_INPUT].value, -5.f, 5.f)+5.f)/10.f);
  if (selectedIndex == activeParams.size())
	selectedIndex--;

  outputs[OUT_OUTPUT].value = getFrequency(activeParams[selectedIndex]);
}


struct CustomScaleWidget : ModuleWidget {
  CustomScaleWidget(CustomScale *module) : ModuleWidget(module) {
	
	setPanel(SVG::load(assetPlugin(plugin, "res/CustomScale.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

	addInput(Port::create<PJ301MPort>(Vec(5, 30), Port::INPUT, module, CustomScale::SIGNAL_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(5, 130), Port::INPUT, module, CustomScale::TONE_INPUT));
	addInput(Port::create<PJ301MPort>(Vec(5, 160), Port::INPUT, module, CustomScale::TOGGLE_TRIGGER_INPUT));

	addOutput(Port::create<PJ301MPort>(Vec(5, 60), Port::OUTPUT, module, CustomScale::OUT_OUTPUT));

	for (int octave=0; octave<CustomScale::NUM_OCTAVES; octave++) 
	  for (int tone=0; tone<12; tone++) {
		float x = 35 + octave * 26;
		float y = 5+26*(12-tone);
		int index = octave*12 + tone;
		
		addParam(ParamWidget::create<LEDBezel>(Vec(x, y), module, index, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MuteLight<GreenLight>>(Vec(x, y), module, index));
	  }
  };
};


Model *modelCustomScale = Model::create<CustomScale, CustomScaleWidget>("noobhour", "customscale", "CustomScale", RANDOM_TAG, DUAL_TAG);
