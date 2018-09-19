#include "Noobhour.hpp"
#include "dsp/digital.hpp"


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

  static const int NUM_COLUMNS = 4;  

  SchmittTrigger gateTriggers[NUM_COLUMNS];
  bool isActive[NUM_COLUMNS];

  Baseliner() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
  
  void step() override;

};


void Baseliner::step() {

  for (int i = 0; i < Baseliner::NUM_COLUMNS; i++) {
	float gate = 0.0;
	if (inputs[GATE1_INPUT + i].active)
	  gate = inputs[GATE1_INPUT + i].value;
	
	float modeFloat = params[MODE1_PARAM + i].value;
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
	if (inputs[P1_INPUT + i].active)
	  p_input = clamp(inputs[P1_INPUT + i].value / 10.f, -10.f, 10.f);
	float p = clamp(p_input + params[P1_PARAM + i].value, 0.0f, 1.0f);

	if (mode == MODE_GATE && (1.0 - p < 1e-4)) { // trivial case: gate mode and probability = 1: use signal when gate is on
		useSignal = gate > 1.0f;
	} else { 
	  bool trigger = gateTriggers[i].process(rescale(gate, 0.1f, 2.f, 0.f, 1.f));
	  bool toss = trigger ? (randomUniform() < p) : false;
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
	  if (inputs[SIGNAL1_INPUT + i].active) 
		input = inputs[SIGNAL1_INPUT + i].value;
	  param = params[SIGNAL1_PARAM + i].value;
	  absVal = params[SIGNAL1ABS_PARAM + i].value;
	  lights[SIGNAL1_LIGHT_POS + 2*i].value = 1.0;
	  lights[BASE1_LIGHT_POS + 2*i].value = 0.0;
	} else {
	  if (inputs[BASE1_INPUT + i].active) 
		input = inputs[BASE1_INPUT + i].value;
	  param = params[BASE1_PARAM + i].value;
	  absVal = params[BASE1ABS_PARAM + i].value;
	  lights[SIGNAL1_LIGHT_POS + 2*i].value = 0.0;
	  lights[BASE1_LIGHT_POS + 2*i].value = 1.0;	  
	}
	float output = clamp(input * param + absVal, -5.f, 5.f);
	
	outputs[OUT1_OUTPUT + i].value = output;	
  }

}


struct BaselinerWidget : ModuleWidget {
  BaselinerWidget(Baseliner *module) : ModuleWidget(module) {
	setPanel(SVG::load(assetPlugin(plugin, "res/Baseliner.svg")));

	addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));	

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
	float yOffset = 20.0f;
	float xGrid = 39.0f;
	float yGrid = 32.0f;

	float probOffset = 0.0f; // move probability section a bit further away
	float attenuatorOffset = 5.0f; // move attenuator and corresponding inputs closer to each other
	float fixOffset = 5.0f;

	for (int i=0; i<Baseliner::NUM_COLUMNS; i++) {
	  addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 0 + fixOffset), module, Baseliner::SIGNAL1ABS_PARAM + i, -5.f, 5.f, 0.f));	  	  
	  addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 1 + attenuatorOffset), module, Baseliner::SIGNAL1_PARAM + i, -1.f, 1.f, 1.f));	  
	  addInput(Port::create<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 2), Port::INPUT, module, Baseliner::SIGNAL1_INPUT + i));

	  addChild(ModuleLightWidget::create<SmallLight<GreenRedLight>>(Vec(xOffset + float(i)*xGrid + offsetLight, yOffset + yGrid * 2.78f), module, Baseliner::SIGNAL1_LIGHT_POS + 2*i));	  
	  addInput(Port::create<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 3), Port::INPUT, module, Baseliner::GATE1_INPUT + i));
	  addChild(ModuleLightWidget::create<SmallLight<GreenRedLight>>(Vec(xOffset + float(i)*xGrid + offsetLight, yOffset + yGrid * 3.78f), module, Baseliner::BASE1_LIGHT_POS + 2*i));

	  addInput(Port::create<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 4), Port::INPUT, module, Baseliner::BASE1_INPUT + i));		
	  addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid* 5 - attenuatorOffset), module, Baseliner::BASE1_PARAM + i, -1.f, 1.f, 1.f));
	  addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid* 6 - fixOffset), module, Baseliner::BASE1ABS_PARAM + i, -5.f, 5.f, 0.f));
	  
	  addOutput(Port::create<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetOutput, yOffset + yGrid * 7 - fixOffset + 2), Port::OUTPUT, module, Baseliner::OUT1_OUTPUT + i));
	  
	  addParam(ParamWidget::create<CKSSThree>(Vec(xOffset + float(i)*xGrid + offsetSwitch, yOffset + yGrid * 8 + probOffset), module, Baseliner::MODE1_PARAM + i, 0.0f, 2.0f, 2.0f));
	  addParam(ParamWidget::create<RoundSmallBlackKnob>(Vec(xOffset + float(i)*xGrid + offsetKnob, yOffset + yGrid * 9 + probOffset), module, Baseliner::P1_PARAM + i, 0.0f, 1.0f, 1.0f));
	  addInput(Port::create<PJ301MPort>(Vec(xOffset + float(i)*xGrid + offsetInput, yOffset + yGrid * 10 + probOffset - attenuatorOffset), Port::INPUT, module, Baseliner::P1_INPUT + i));
	}
  }

};


Model *modelBaseliner = Model::create<Baseliner, BaselinerWidget>("noobhour", "baseliner", "Baseliner", RANDOM_TAG, DUAL_TAG);
