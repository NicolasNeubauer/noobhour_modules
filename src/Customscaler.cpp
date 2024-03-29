#include "Noobhour.hpp"
#include <vector>
#include <set>


// TODO LATER
// scale file loader - Latif Fital
// CustomScale Pro
// - paging? different pages, back/forth, CV for index selection (interpolate between non-empty pages, lights to indicate?)
// - quad
// - button back for randomization

// TODO

// IN PROGRESS
// bus

// DONE
// bsl1r
// context menu switch between 0-10 and -5..5 - (was attenuator - Pyer Cllrd)
// Normalizing the gate input to all following inputs (with nothing plugged in them) would be amazing (instead of 4 copies of the same cable) Patrick McIlveen
// Normalized HIGH and LOW inputs as well
// Normalizing all of the outputs to the last one (a la AS 4ch baby mixer and Audible) Patrick McIlveen
// Performance imporvement CustomScale
// random subset (randomize activity of individual tones) - Pyer Cllrd
// ended up with lines on I, IV, V, was: change rings around lights for black/white distinction - steve baker 




struct GreenBlueYellowLight : GrayModuleLightWidget {
  GreenBlueYellowLight() {
	addBaseColor(SCHEME_GREEN);
	addBaseColor(SCHEME_BLUE);
	addBaseColor(SCHEME_YELLOW);		
  }
};

template <typename BASE>
struct ToneLight : BASE {
  ToneLight() {
	this->box.size = mm2px(Vec(6.0f, 6.0f));
  }
};

struct Customscaler : Module {

  static const int NUM_OCTAVES = 5;
  static const int BASE_OCTAVE = 2;
  static const int NUM_TONES = NUM_OCTAVES * 12;
  
  enum InputIds {
	SIGNAL_INPUT,
	TONE_INPUT,
	TOGGLE_TRIGGER_INPUT,
	RESET_TRIGGER_INPUT,
	RANDOMIZE_TRIGGER_INPUT,
	P_INPUT,
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
	// RANDOMIZE_BUTTON_PARAM,
	RESET_BUTTON_PARAM,
	BASE_PARAM,
	MODE_PARAM,
	
	NUM_PARAMS
  };

  enum LightIds {
	ENUMS(TONE1_LIGHT, NUM_TONES * 3),

	NUM_LIGHTS
  };

  dsp::SchmittTrigger gateTrigger[NUM_TONES];
  dsp::SchmittTrigger randomizeTrigger;
  dsp::SchmittTrigger resetTrigger;
  dsp::SchmittTrigger resetButtonTrigger;
  dsp::SchmittTrigger paramTrigger[NUM_TONES];
  
  dsp::PulseGenerator changePulse[PORT_MAX_CHANNELS];
  
  bool state[NUM_TONES];
  bool candidate[NUM_TONES];
  int lastFinalTone[PORT_MAX_CHANNELS];
  int lastStartTone = NUM_TONES;
  int lastSelectedTone[PORT_MAX_CHANNELS]; 

  std::vector<int> activeTones;
  bool activeTonesDirty = true;

  bool bipolarInput = false;


  Customscaler() {
	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	activeTones.reserve(NUM_TONES);
	configParam(Customscaler::RANGE_PARAM, 0.f, 2.f, 0.f, "Range", "octaves");		
	configParam(Customscaler::BASE_PARAM, 0.f, 11.f, 0.f, "Base tone", "half tone");	
	configParam(Customscaler::P_PARAM, 0.f, 1.f, 0.5f, "Probability", "%");		
	configParam(Customscaler::MODE_PARAM, 0.f, 1.f, 1.f, "Mode");	
	configParam(Customscaler::RESET_BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "Reset");	
	for (int octave=0; octave<Customscaler::NUM_OCTAVES; octave++) {
	  for (int tone=0; tone<12; tone++) {
		int index = octave * 12 + tone;
		configParam(Customscaler::TONE1_PARAM + index, 0.0f, 1.0f, 0.0f, octave == 2 && tone==0?"C4":"");
	  }
	}
	for (int c=0; c<PORT_MAX_CHANNELS; c++) {
	  lastSelectedTone[c] = NUM_TONES;
	  lastFinalTone[c] = NUM_TONES;
	}
	onReset();	
  }
  
  void process(const ProcessArgs &args) override;

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
	  candidate[i] = false;
	}
	activeTonesDirty = true;
  }

  float getP() {
	float p_input = 0;
	if (inputs[P_INPUT].isConnected())
	  p_input = clamp(inputs[P_INPUT].getVoltage() / 10.f, -10.f, 10.f);
	return clamp(p_input + params[P_PARAM].getValue(), 0.0f, 1.0f);
  }
  
  void onRandomize() override {
	randomizeTones(getP()); 
  }

  void randomizeTones(float p) {
	for (int i = 0; i < NUM_TONES; i++) {
	  state[i] = (random::uniform() < p);
	  candidate[i] = false;
	}
	activeTonesDirty = true;
  }

  void randomSubset(float p) {
	int activeTones = 0;
	int candidates = 0;
	bool toggle = params[MODE_PARAM].getValue() < 0.5f;
	for (int i = 0; i < NUM_TONES; i++) {
	  if (state[i] || candidate[i]) {
		candidates++;

		if (toggle) {
		  if (random::uniform() < p) {
			state[i] ^= true;
		  }
		  if (state[i]) {
			activeTones++;
			candidate[i] = false;
		  } else {
			candidate[i] = true;
		  }
		} else {		
		  if (random::uniform() < p) {
			activeTones++;
			state[i] = true;
			candidate[i] = false;
		  } else {
			state[i] = false;
			candidate[i] = true;
		  }
		}
	  }
	}

	// if random subset is called without active or candidate tones,
	// let it behave like the normal randomisation: everything is a
	// candidate, retry
	if (candidates == 0) {
	  for (int i = 0; i < NUM_TONES; i++) {
		candidate[i] = true;
	  }
	  randomSubset(p);
	  return;
	}

	// make sure at least one tone is active so we don't return 0 = C4
	// which may be not be a candidate
	if (activeTones == 0) {
	  for (int i = 0; i < NUM_TONES; i++) {
		if (candidate[i]) {
		  state[i] = true;
		  candidate[i] = false;
		  break;
		}
	  }
	}
	activeTonesDirty = true;	
  }

  json_t *dataToJson() override {
	json_t *rootJ = json_object();

	json_t *statesJ = json_array();
	for (int i = 0; i < NUM_TONES; i++) {
	  json_t *stateJ = json_boolean(state[i]);
	  json_array_append_new(statesJ, stateJ);
	}
	json_object_set_new(rootJ, "states", statesJ);

	json_t *candidatesJ = json_array();
	for (int i = 0; i < NUM_TONES; i++) {
	  json_t *candidateJ = json_boolean(candidate[i]);
	  json_array_append_new(candidatesJ, candidateJ);
	}
	json_object_set_new(rootJ, "candidates", candidatesJ);
	
	json_t *bipolarInputJ = json_boolean(bipolarInput);
	json_object_set_new(rootJ, "bipolarInput", bipolarInputJ);
	
	return rootJ;
  }
  
  void dataFromJson(json_t *rootJ) override {
	json_t *statesJ = json_object_get(rootJ, "states");
	if (statesJ) {
	  for (int i = 0; i < NUM_TONES; i++) {
		json_t *stateJ = json_array_get(statesJ, i);
		if (stateJ)
		  state[i] = json_boolean_value(stateJ);
	  }
	}

	json_t *candidatesJ = json_object_get(rootJ, "candidates");
	if (candidatesJ) {
	  for (int i = 0; i < NUM_TONES; i++) {
		json_t *candidateJ = json_array_get(candidatesJ, i);
		if (candidateJ)
		  candidate[i] = json_boolean_value(candidateJ);
	  }
	}

	json_t *bipolarInputJ = json_object_get(rootJ, "bipolarInput");
	bipolarInput = json_boolean_value(bipolarInputJ);
	  
	
	activeTonesDirty = true;
  }

};




void Customscaler::process(const ProcessArgs &args) {
  // RESET
  if (inputs[RESET_TRIGGER_INPUT].isConnected()) {
	if (resetTrigger.process(rescale(inputs[RESET_TRIGGER_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
	  onReset();
	}	
  }

  if (resetButtonTrigger.process(params[RESET_BUTTON_PARAM].getValue())) {
	onReset();
  }
  

  // RANDOMIZE
  if (inputs[RANDOMIZE_TRIGGER_INPUT].isConnected()) {
	if (randomizeTrigger.process(rescale(inputs[RANDOMIZE_TRIGGER_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
	  randomSubset(getP());
	}		
  }

  /*
  if (randomizeButtonTrigger.process(params[RANDOMIZE_BUTTON_PARAM].getValue())) {
	randomizeTones(getP());
  }
  */

  // TOGGLE
  if (inputs[TONE_INPUT].isConnected()) {
	int channels = inputs[TONE_INPUT].getChannels();
	for (int c = 0; c < channels; c++) {	
	  float gate = 0.0;
	  if (inputs[TOGGLE_TRIGGER_INPUT].isConnected())
		gate = inputs[TOGGLE_TRIGGER_INPUT].getVoltage(c);
	  if (gateTrigger[c].process(rescale(gate, 0.1f, 2.f, 0.f, 1.f))) {
		int toneIndex = getTone(inputs[TONE_INPUT].getVoltage(c));
		if (toneIndex >= 0 && toneIndex < NUM_TONES) {
		  state[toneIndex] ^= true;
		  candidate[toneIndex] = false;
		  activeTonesDirty = true;
		}
	  }
	}
  }

  // OCTAVE RANGE
  int startTone = 0;
  int endTone = 0;
  if (params[RANGE_PARAM].getValue() < 0.5f) {
	startTone = 0;
	endTone = NUM_TONES - 1;
  } else if (params[RANGE_PARAM].getValue() < 1.5f) {
	startTone = 12;
	endTone = NUM_TONES - 13;
  } else {
	startTone = 24;
	endTone = NUM_TONES - 25;
  }
  if (startTone != lastStartTone) {
	activeTonesDirty = true;
	lastStartTone = startTone;
  }

  // CHECK TONE TOGGLES
  for (int i = 0; i < NUM_TONES; i++) {
	if (paramTrigger[i].process(params[i].getValue())) {
	  state[i] ^= true;
	  candidate[i] = false;
	  activeTonesDirty = true;
	}
  }

  // GATHER CANDIDATES
  if (activeTonesDirty) {	
	activeTones.clear();
	for (int i = 0; i < NUM_TONES; i++) {
	  if (state[i] && i >= startTone && i <= endTone) {
		activeTones.push_back(i);
	  } 	 
	}
  }

  // FETCH BASE TONE
  float baseTone = params[BASE_PARAM].getValue();
  if (inputs[BASE_INPUT].isConnected()) {
	baseTone +=  inputs[BASE_INPUT].getVoltage() / 10.f * 11.f;
  }
  int baseToneDiscrete = static_cast<int>(clamp(baseTone, 0.f, 11.f));

  // SELECT TONE  
  float output = 0;
  int selectedTone;
  bool oneSelectedToneDirty = false;
  std::set<int> selectedTones;
	

  

  /* POLYPHONY
	 only matters below (above is polyphony-agnostic manipulation of the module's state)

	 UPDATE polyphony also matters above now when toggling

	 if input is polyphonic, output and changegate are polyphonic too; different inputs are 
	 mapped to different tones according to the same scale however. 
	 whenever one signal

	 lastFinalTone needs to be an array too so we know when to trigger a change gate
  */
  
  if (inputs[SIGNAL_INPUT].isConnected() && activeTones.size() > 0) {
	int channels = inputs[SIGNAL_INPUT].getChannels();
	for (int c = 0; c < channels; c++) {
	  float inp = inputs[SIGNAL_INPUT].getVoltage(c);
	  if (bipolarInput)
		inp += 5.f;
	  unsigned int selectedIndex = static_cast<int>(activeTones.size() * (clamp(inp, 0.f, 10.f)) / 10.f);
	  if (selectedIndex == activeTones.size())
		selectedIndex--;
	  selectedTone = activeTones[selectedIndex];
	  selectedTones.insert(selectedTone);
	  if (selectedTone != lastSelectedTone[c]) {
		lastSelectedTone[c] = selectedTone;
		oneSelectedToneDirty = true;
	  }
	  int finalTone = selectedTone + baseToneDiscrete;
	  output = getVOct(finalTone);
	  outputs[OUT_OUTPUT].setVoltage(output, c);
	  // DETECT TONE CHANGE
	  if (finalTone != lastFinalTone[c]) {
		changePulse[c].trigger(0.001f);
		lastFinalTone[c] = finalTone;
	  }  
	  outputs[CHANGEGATE_OUTPUT].setVoltage((changePulse[c].process(1.0f / args.sampleRate) ? 10.0f : 0.0f), c);
	}
	outputs[OUT_OUTPUT].setChannels(channels);
	outputs[CHANGEGATE_OUTPUT].setChannels(channels);

  }
  


  // LIGHTS
  if (activeTonesDirty || oneSelectedToneDirty) {
	oneSelectedToneDirty = false;
	for (int i = 0; i < NUM_TONES; i++) {
	  float green = 0.f;
	  float blue = 0.f;
	  float yellow = 0.f;
	  if (state[i]) {
		if (selectedTones.find(i) != selectedTones.end()) {
		  blue = 0.9f;
		} else {
		  if (i >= startTone && i <= endTone) {
			green = 0.9f; // active tone but not selected
		  } else {
			green = 0.1f; // active but in inactive octave
		  }		
		}
	  } else {
		if (candidate[i]) {
		  if (i >= startTone && i <= endTone) {
			yellow = 0.3f; // candidate 
		  } else {
			yellow = 0.1f; // candidate but in inactive octave
		  }		
		}		  
	  }
	  lights[i * 3].setBrightness(green);
	  lights[i * 3 + 1].setBrightness(blue);
	  lights[i * 3 + 2].setBrightness(yellow);	  
	}
  }
  activeTonesDirty = false; // only reset after check for lights has been done
}


struct CustomscalerWidget : ModuleWidget {
  // generate controls	
  const int yStart = 25;
  const int yRange = 40;
  const int ySeparator = 5;
  const float x = 11.5f;
  const float x2 = 46.5f;
  const float lastY = 329;

  // static const float wKnob = 30.23437f;
  const float wInput = 31.58030f;
  const float wSwitch = 17.94267f;	
  const float offsetKnob = -2.1; 
  const float offsetSwitch = (wInput - wSwitch) / 2.0f - 1.5; // no idea why 1.5, not centered otherwise
  const int offsetTL1005 = 4;

  const bool whiteKey[12] = {true, false, true, false, true, true, false, true, false, true, false, true};
  
  CustomscalerWidget(Customscaler *module) {
	setModule(module);

	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CustomScaler.svg")));
	addChild(createWidget<ScrewSilver>(Vec(15, 0)));
	addChild(createWidget<ScrewSilver>(Vec(15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x-30, 365)));

	// upper panel

	addInput(createInput<PJ301MPort>(Vec(x, yStart + 0 * yRange + 0 * ySeparator), module, Customscaler::SIGNAL_INPUT));
	addParam(createParam<CKSSThree>(Vec(x2 + offsetSwitch, yStart + 0 * yRange + 0 * ySeparator), module, Customscaler::RANGE_PARAM));
	
	addInput(createInput<PJ301MPort>(Vec(x, yStart + 1 * yRange + 1 * ySeparator), module, Customscaler::BASE_INPUT));
	addParam(createParam<RoundBlackSnapKnob>(Vec(x2 + offsetKnob, yStart + 1 * yRange + 1 * ySeparator + offsetKnob), module, Customscaler::BASE_PARAM));		
	
	addOutput(createOutput<PJ301MPort>(Vec(x, yStart + 2 * yRange + 2 * ySeparator), module, Customscaler::OUT_OUTPUT));
	addOutput(createOutput<PJ301MPort>(Vec(x2, yStart + 2 * yRange + 2 * ySeparator), module, Customscaler::CHANGEGATE_OUTPUT));	
	
	// lower panel
	
	addInput(createInput<PJ301MPort>(Vec(x, lastY - (3 * yRange + 2 * ySeparator)), module, Customscaler::TONE_INPUT));
	addInput(createInput<PJ301MPort>(Vec(x2, lastY - (3 * yRange + 2 * ySeparator)), module, Customscaler::TOGGLE_TRIGGER_INPUT)); 

	// addInput(createInput<PJ301MPort>(Vec(x, lastY - (1 * yRange + 1 * ySeparator)), module, Customscaler::RANDOM_SUBSET_TRIGGER_INPUT));
	// configParam(Customscaler::RANDOMIZE_BUTTON_PARAM, 0.0f, 1.0f, 0.0f, "");
	// addParam(createParam<TL1105>(Vec(x2 + offsetTL1005, lastY - (3 * yRange + 1 * ySeparator - offsetTL1005)), module, Customscaler::RANDOMIZE_BUTTON_PARAM));

	addParam(createParam<RoundBlackKnob>(Vec(x2 + offsetKnob, lastY - (2 * yRange + 1 * ySeparator - offsetKnob)), module, Customscaler::P_PARAM));
	addInput(createInput<PJ301MPort>(Vec(x, lastY - (2 * yRange + 1 * ySeparator)), module, Customscaler::P_INPUT));

	addInput(createInput<PJ301MPort>(Vec(x, lastY - (1 * yRange + 1 * ySeparator)), module, Customscaler::RANDOMIZE_TRIGGER_INPUT));
	addParam(createParam<CKSS>(Vec(x2 + offsetSwitch, lastY - (1 * yRange + 1 * ySeparator)), module, Customscaler::MODE_PARAM));

	addInput(createInput<PJ301MPort>(Vec(x, lastY), module, Customscaler::RESET_TRIGGER_INPUT)); 
	addParam(createParam<TL1105>(Vec(x2 + offsetTL1005, lastY  + offsetTL1005), module, Customscaler::RESET_BUTTON_PARAM)); 
	
	// generate lights
	float offsetX = mm2px(Vec(17.32, 18.915)).x - mm2px(Vec(16.57, 18.165)).x - 0.5; // from Mutes
	float offsetY = mm2px(Vec(17.32, 18.915)).y - mm2px(Vec(16.57, 18.165)).y - 0.5;	
	for (int octave=0; octave<Customscaler::NUM_OCTAVES; octave++) {
	  float x = 88 + octave * 27;
	  for (int tone=0; tone<12; tone++) {
		float y = -5 + 28 * (12 - tone);
		int index = octave * 12 + tone;

		addParam(createParam<LEDBezel>(Vec(x, y), module, Customscaler::TONE1_PARAM + index)); 
		addChild(createLight<ToneLight<GreenBlueYellowLight>>(Vec(x + offsetX, y + offsetY), module, Customscaler::TONE1_PARAM + index * 3)); 
	  }
	}
  };


  struct UniBiItem : MenuItem {
	Customscaler *module;
	void onAction(const event::Action &e) override {
	  module->bipolarInput ^= true;;
	}
  };

  void appendContextMenu(Menu *menu) override {
	Customscaler *module = dynamic_cast<Customscaler*>(this->module);	
	
	menu->addChild(new MenuEntry);
	UniBiItem *uniBiItem = createMenuItem<UniBiItem>("bipolar input", CHECKMARK(module->bipolarInput));
	uniBiItem->module = module;
	menu->addChild(uniBiItem);
  }
  
};


Model *modelCustomscaler = createModel<Customscaler, CustomscalerWidget>("customscale");
