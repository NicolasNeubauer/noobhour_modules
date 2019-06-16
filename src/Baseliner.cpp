#include "Noobhour.hpp"
#include <algorithm> 

template <int NUM_COLUMNS>
struct Baseliner : Module {
  enum ParamIds {
	SIGNAL1ABS_PARAM,
	SIGNAL2ABS_PARAM,
	SIGNAL3ABS_PARAM,
	SIGNAL4ABS_PARAM,
	
	SIGNAL1_PARAM,
	SIGNAL2_PARAM,
	SIGNAL3_PARAM,
	SIGNAL4_PARAM,
	
	BASE1ABS_PARAM,
	BASE2ABS_PARAM,
	BASE3ABS_PARAM,
	BASE4ABS_PARAM,

	BASE1_PARAM,
	BASE2_PARAM,
	BASE3_PARAM,
	BASE4_PARAM,
	  
	MODE1_PARAM,
	MODE2_PARAM,
	MODE3_PARAM,
	MODE4_PARAM,
	  
	P1_PARAM,
	P2_PARAM,
	P3_PARAM,
	P4_PARAM,
	  
	NUM_PARAMS	  
  };
  
  enum InputIds {
	SIGNAL1_INPUT,
	SIGNAL2_INPUT,
	SIGNAL3_INPUT,
	SIGNAL4_INPUT,
	  
	BASE1_INPUT,
	BASE2_INPUT,
	BASE3_INPUT,
	BASE4_INPUT,
	  
	GATE1_INPUT,
	GATE2_INPUT,
	GATE3_INPUT,
	GATE4_INPUT,
	  
	P1_INPUT,
	P2_INPUT,
	P3_INPUT,
	P4_INPUT,
	  
	NUM_INPUTS
  };
  
  enum OutputIds {
	OUT1_OUTPUT,
	OUT2_OUTPUT,
	OUT3_OUTPUT,
	OUT4_OUTPUT,
	  
	NUM_OUTPUTS	  
  };
  
  enum LightIds {
	SIGNAL1_LIGHT_POS, SIGNAL1_LIGHT_NEG,
	SIGNAL2_LIGHT_POS, SIGNAL2_LIGHT_NEG,
	SIGNAL3_LIGHT_POS, SIGNAL3_LIGHT_NEG,
	SIGNAL4_LIGHT_POS, SIGNAL4_LIGHT_NEG,

	BASE1_LIGHT_POS, BASE1_LIGHT_NEG,
	BASE2_LIGHT_POS, BASE2_LIGHT_NEG,
	BASE3_LIGHT_POS, BASE3_LIGHT_NEG,
	BASE4_LIGHT_POS, BASE4_LIGHT_NEG,

	NUM_LIGHTS
  };

  enum Modes {
	MODE_GATE,
	MODE_TOGGLE,
	MODE_LATCH
  };

  dsp::SchmittTrigger gateTriggers[NUM_COLUMNS];
  bool isActive[NUM_COLUMNS];

  Baseliner() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	for (int i = 0; i < NUM_COLUMNS; i++) {
	  configParam(Baseliner<NUM_COLUMNS>::SIGNAL1ABS_PARAM + i, -5.f, 5.f, 0.f, "Absolute value HIGH", "V");
	  configParam(Baseliner<NUM_COLUMNS>::SIGNAL1_PARAM + i, -1.f, 1.f, 1.f, "Attenuation HIGH", "*");	  
	  configParam(Baseliner<NUM_COLUMNS>::BASE1_PARAM + i, -1.f, 1.f, 1.f, "Attenuation LOW", "*");
	  configParam(Baseliner<NUM_COLUMNS>::BASE1ABS_PARAM + i, -5.f, 5.f, 0.f, "Absolute value LOW", "V");
	  configParam(Baseliner<NUM_COLUMNS>::MODE1_PARAM + i, 0.0f, 2.0f, 2.0f, "Mode");
	  configParam(Baseliner<NUM_COLUMNS>::P1_PARAM + i, 0.0f, 1.0f, 1.0f, "Probability");
	}
	
  }
  
  void process(const ProcessArgs &args) override;

};

template <int NUM_COLUMNS>
void Baseliner<NUM_COLUMNS>::process(const ProcessArgs &args) {
  float outputs_cache[NUM_COLUMNS][PORT_MAX_CHANNELS] = {0};
  int channels[NUM_COLUMNS] = {0};
  int maxChannels = 0;
  for (int i = 0; i < NUM_COLUMNS; i++) {
	float gate = 0.0;

	// If gate isn't active, use an earlier, active gate's input (daisy-chaining)
	for (int j = i; j >= 0; j--) {
	  if (inputs[GATE1_INPUT + j].isConnected()) {
		gate = inputs[GATE1_INPUT + j].getVoltage();
		break;
	  }
	}
	
	float modeFloat = params[MODE1_PARAM + i].getValue();
	int mode = 0;
	if (modeFloat > 1.5) {
	  mode = MODE_GATE;
	} else if (modeFloat > 0.5) {
	  mode = MODE_LATCH;
	} else {
	  mode = MODE_TOGGLE;
	}
	
	bool useSignal = false;
	float p_input = 0;
	if (inputs[P1_INPUT + i].isConnected())
	  p_input = clamp(inputs[P1_INPUT + i].getVoltage() / 10.f, -10.f, 10.f);
	float p = clamp(p_input + params[P1_PARAM + i].getValue(), 0.0f, 1.0f);

	if (mode == MODE_GATE && (1.0 - p < 1e-4)) { // trivial case: gate mode and probability = 1: use signal when gate is on
	  useSignal = gate > 1.0f;
	} else { 
	  bool trigger = gateTriggers[i].process(rescale(gate, 0.1f, 2.f, 0.f, 1.f));
	  bool toss = trigger ? (random::uniform() < p) : false;
	  switch (mode) {
		
	  case MODE_GATE:
		if (trigger) {
		  isActive[i] = toss; 
		}
		useSignal = isActive[i] && gate > 1.0f;
		break;

	  case MODE_LATCH:
		if (trigger) {
		  isActive[i] = toss;
		}
		useSignal = isActive[i];
		break;	  
		
		
	  case MODE_TOGGLE:
		if (trigger && toss) {
		  isActive[i] = !isActive[i];
		}
		useSignal = isActive[i];
		break;
	  }
	}

	float input = 0.0f;
	float param = 0.0f;
	float absVal = 0.0f;
	
	if (useSignal) {
	  channels[i] = inputs[SIGNAL1_INPUT + i].getChannels();
	  param = params[SIGNAL1_PARAM + i].getValue();
	  absVal = params[SIGNAL1ABS_PARAM + i].getValue();
	  lights[SIGNAL1_LIGHT_POS + 2*i].value = 1.0;
	  lights[BASE1_LIGHT_POS + 2*i].value = 0.0;
	} else {
	  channels[i] = inputs[BASE1_INPUT + i].getChannels();
	  param = params[BASE1_PARAM + i].getValue();
	  absVal = params[BASE1ABS_PARAM + i].getValue();
	  lights[SIGNAL1_LIGHT_POS + 2*i].value = 0.0;
	  lights[BASE1_LIGHT_POS + 2*i].value = 1.0;	  
	}
	maxChannels = std::max(maxChannels, channels[i]);
	
	for (int c=0; c<channels[i]; c++) {
	  if (useSignal) {
		input = inputs[SIGNAL1_INPUT + i].getVoltage(c);
	  } else {
		input = inputs[BASE1_INPUT + i].getVoltage(c);
	  }
	  float output = clamp(input * param + absVal, -10.f, 10.f);
	  outputs_cache[i][c] = output;	  
	}
  }

  // daisy-chain outputs
  float stacked[PORT_MAX_CHANNELS] = {0.f};
  
  int currentChannels = 0;
  for (int i=0; i < NUM_COLUMNS; i++) {
	currentChannels = std::max(currentChannels, channels[i]);
	if (outputs[OUT1_OUTPUT + i].isConnected()) {
	  for (int c=0; c < currentChannels; c++) {
		// if only one channel, distribute that value over all channels
		float currentVoltage = channels[i] == 1 ? outputs_cache[i][0] : outputs_cache[i][c];
		float voltage = stacked[c] + currentVoltage;
		outputs[OUT1_OUTPUT + i].setVoltage(voltage, c);
	  }
	  outputs[OUT1_OUTPUT + i].setChannels(currentChannels);
	  std::fill(stacked, stacked+PORT_MAX_CHANNELS, 0);
	  currentChannels = 0;
	} else {
	  for (int c=0; c < maxChannels; c++) {
		if (channels[i]==1)  // if only one channel, distribute that value over all channels
		  stacked[c] += outputs_cache[i][0];		  
		else
		  stacked[c] += outputs_cache[i][c];
	  }
	}
  }


}

template <int NUM_COLUMNS>
struct BaselinerWidget : ModuleWidget {
  BaselinerWidget(Baseliner<NUM_COLUMNS> *module) {

	setModule(module);
	std::string filename = (NUM_COLUMNS == 1 ? "res/Bsl1r.svg" : "res/Baseliner.svg");
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, filename)));

	if (NUM_COLUMNS > 1) {
	  addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	  addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	  addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	  addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));
	}

	float wKnob = 30.23437f;
	float wSwitch = 17.94267f;
	float wInput = 31.58030f;
	float wOutput = wInput; // 39.13760f;
	float wLight = 2.176f;

	float offsetKnob = (wOutput - wKnob) / 2.0f;
	float offsetSwitch = (wOutput - wSwitch) / 2.0f - 1.5; // no idea why 1.5, not centered otherwise
	float offsetInput = (wOutput - wInput) / 2.0f;
	float offsetOutput = (wOutput - wOutput) / 2.0f;
	float offsetLight = (wOutput - wLight) / 2.0f - 5.5; // no idea why 5.5, not centered otherwise
		
	float xOffset = 30.0f;
	if (NUM_COLUMNS == 1) {
	  xOffset = 2;
	}
	float yOffset = 20.0f;
	float xGrid = 39.0f;
	float yGrid = 32.0f;

	float probOffset = 0.0f; // move probability section a bit further away
	float attenuatorOffset = 5.0f; // move attenuator and corresponding inputs closer to each other
	float fixOffset = 5.0f;

	for (int i=0; i<NUM_COLUMNS; i++) {

	  addParam(createParam<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 0 + fixOffset), module, Baseliner<NUM_COLUMNS>::SIGNAL1ABS_PARAM + i));
	  addParam(createParam<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 1 + attenuatorOffset), module, Baseliner<NUM_COLUMNS>::SIGNAL1_PARAM + i));	  
	  addInput(createInput<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 2), module, Baseliner<NUM_COLUMNS>::SIGNAL1_INPUT + i));
	  
	  addChild(createLight<SmallLight<GreenRedLight>>(Vec(xOffset + float(i)*xGrid + offsetLight, yOffset + yGrid * 2.78f), module, Baseliner<NUM_COLUMNS>::SIGNAL1_LIGHT_POS + 2*i));	  
	  addInput(createInput<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 3), module, Baseliner<NUM_COLUMNS>::GATE1_INPUT + i));
	  addChild(createLight<SmallLight<GreenRedLight>>(Vec(xOffset + float(i)*xGrid + offsetLight, yOffset + yGrid * 3.78f), module, Baseliner<NUM_COLUMNS>::BASE1_LIGHT_POS + 2*i));

	  addInput(createInput<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 4), module, Baseliner<NUM_COLUMNS>::BASE1_INPUT + i));
	  addParam(createParam<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid* 5 - attenuatorOffset), module, Baseliner<NUM_COLUMNS>::BASE1_PARAM + i));
	  addParam(createParam<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid* 6 - fixOffset), module, Baseliner<NUM_COLUMNS>::BASE1ABS_PARAM + i));
	  
	  addOutput(createOutput<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetOutput, yOffset + yGrid * 7 - fixOffset + 2), module, Baseliner<NUM_COLUMNS>::OUT1_OUTPUT + i));

	  addParam(createParam<CKSSThree>(Vec(xOffset + float(i)*xGrid + offsetSwitch, yOffset + yGrid * 8 + probOffset), module, Baseliner<NUM_COLUMNS>::MODE1_PARAM + i));
	  addParam(createParam<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 9 + probOffset), module, Baseliner<NUM_COLUMNS>::P1_PARAM + i));
	  addInput(createInput<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 10 + probOffset - attenuatorOffset), module, Baseliner<NUM_COLUMNS>::P1_INPUT + i));
	}
  }

};


Model *modelBaseliner = createModel<Baseliner<4>, BaselinerWidget<4>>("baseliner");
Model *modelBsl1r = createModel<Baseliner<1>, BaselinerWidget<1>>("bsl1r");
