#include "LedAnimation.h"

void LedAnimation::UpdatePresetDefinition(int presetId, std::string definitionString)
{
	if (presetId<0 || presetId>=LEDANIMATION_PRESETS_COUNT){
		 return;
	}
	int StepIdx = 0;
    static LedAnimationDefinitionParser parser = LedAnimationDefinitionParser();
    std::vector<std::vector<int>> data = parser.interpretDefinitionString(definitionString, "1000,100,0,0,0,0,0");

    std::vector<LedStripTransition> steps;
    for (const auto& transStepData : data) {
        LedStripTransition step = LedStripTransition(
			LedStripState(
				transStepData[1],
				transStepData[2],
				transStepData[3],
				transStepData[4],
				transStepData[5],
				transStepData[6]
			),
            transStepData[0]
		);
        steps.push_back(step);
    }
    this->UpdatePresetDefinition(presetId, steps);
}

void LedAnimation::UpdatePresetDefinition(int presetId, std::vector<LedStripTransition> steps)
{
	//prepare new preset data structure in mem
	LedStripAnimationPreset *newPreset = (LedStripAnimationPreset*)malloc(sizeof(LedStripAnimationPreset));
	newPreset->allocated = true;
	newPreset->countOfSteps = steps.size();
	newPreset->steps = (LedStripTransition *)malloc(sizeof(LedStripTransition) * steps.size());
	std::copy(steps.begin(), steps.end(), newPreset->steps);
	steps.clear();

	bool isActiveNow = (presetId + LEDANIMATION_PRESETS_TO_MODE_OFFSET) == this->current_mode;
	if (isActiveNow) {
		this->animation_state.active = false; //ensure is off before swap
	}

	//free mem if previous was dynamic
	if (this->presets[presetId] != nullptr && this->presets[presetId]->allocated)
    {
		free(this->presets[presetId]->steps);
		this->presets[presetId]->steps = nullptr;
		free(this->presets[presetId]);
		this->presets[presetId] = nullptr;
	}
	//update
	this->presets[presetId] = newPreset;
	
	if (isActiveNow) {
		this->AnimationStart(presetId);
	}
}

void LedAnimation::setTimeCallback(unsigned long (*callback)(void))
{
    this->timeCallback = callback;
}

//ms absolute time
unsigned long LedAnimation::GetCurrentTime()
{
    if (this->timeCallback == nullptr) {
        return 0;
    }
    return this->timeCallback();
}

void LedAnimation::SetLevel(int newValue)
{
	if (newValue<0) newValue=0;
	if (newValue>100) newValue=100;
	this->target_level = newValue;
}

void LedAnimation::ChangeLevel(int relativeValue)
{
    this->SetLevel((int)this->target_level + relativeValue);
}

void LedAnimation::SetMode(int newMode)
{
	if (this->current_mode == newMode){
		return;
	}
	switch (newMode)
	{
		case 0:
			break;
		case 1:
			this->animation_state.active = false;
			this->StartTransition(LedStripTransition(this->mode1_state_ledStrip, this->default_transition_time));
			break;
		default:
			this->AnimationStart(newMode - LEDANIMATION_PRESETS_TO_MODE_OFFSET);
			break;
	}
    this->current_mode = newMode;
}

void LedAnimation::UpdateMode1(LedStripState newState)
{
    this->mode1_state_ledStrip =  newState;
	if (this->current_mode == 1){
		this->StartTransition(LedStripTransition(this->mode1_state_ledStrip, this->default_transition_time));
	}
}

bool LedAnimation::AnimationStart(int presetId)
{
	if (presetId<0 || presetId>=LEDANIMATION_PRESETS_COUNT || this->presets[presetId] == nullptr){
		 return false;
	}
	this->animation_state.preset = this->presets[presetId];

	if (this->animation_state.preset->countOfSteps>0) {
		this->animation_state.active = true;
		this->animation_state.currentStep = this->animation_state.preset->countOfSteps-1;//set the end, for start:
		this->AnimationNextStep();
		this->current_mode = presetId + LEDANIMATION_PRESETS_TO_MODE_OFFSET;
	}
	return true;
}

void LedAnimation::AnimationNextStep(unsigned long startTime)
{
    this->animation_state.currentStep++;
	if (this->animation_state.currentStep >= this->animation_state.preset->countOfSteps  ) {
		this->animation_state.currentStep = 0;
	}
	this->StartTransition(this->animation_state.preset->steps[this->animation_state.currentStep], startTime);
}

void LedAnimation::StartTransition(LedStripTransition transitionStep, unsigned long startTime)
{
	this->transition_state.transitionStep = transitionStep;
	this->transition_state.startTime = (startTime == 0) ? this->GetCurrentTime() : startTime;
	this->transition_state.startState = this->current_state_ledStrip;
	this->transition_state.active = true;
}

int LedAnimation::loopTick(LedStripState *output ) 
{
    static int update;
    static int factic_target_level; ;

    update = 0;


	if (this->transition_state.active){
		unsigned long now = this->GetCurrentTime();

		float timeRelativePosition = (float)(now - this->transition_state.startTime) / (float)this->transition_state.transitionStep.transitionTime;

		if (timeRelativePosition>=0.0f){
			if (timeRelativePosition>=1.0f){ //transition is over
				this->transition_state.updateState(1.0f, &this->current_state_ledStrip);
				this->transition_state.active = false;
				if (this->animation_state.active){
					unsigned long endTime = this->transition_state.startTime + this->transition_state.transitionStep.transitionTime;
					this->AnimationNextStep(endTime);
					return loopTick(output);//like restart, we are in next step, so start again
				}
			}else if (timeRelativePosition>=0.0f){//in transition
				this->transition_state.updateState(timeRelativePosition, &this->current_state_ledStrip);
			}
			update = 1;
		}
	}

	factic_target_level = (this->current_mode>0) ?  this->target_level : 0;
	
	if (factic_target_level != this->current_level){
		int stepSize = (int)factic_target_level - this->current_level;
		if (stepSize>2) stepSize = 2;
		if (stepSize<-2) stepSize = -2;
		this->current_level += stepSize;
        update = 1;
	}

    if (update){
        output->r = this->current_state_ledStrip.rLeveled(this->current_level);
        output->g = this->current_state_ledStrip.gLeveled(this->current_level);
        output->b = this->current_state_ledStrip.bLeveled(this->current_level);
        output->ww = this->current_state_ledStrip.wwLeveled(this->current_level);
        output->cw = this->current_state_ledStrip.cwLeveled(this->current_level);
        return 1;
    }
    return 0;
}

std::string _LA_trim(std::string str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(),
        [](int ch) { return !std::isspace(ch); }));

    str.erase(std::find_if(str.rbegin(), str.rend(),
        [](int ch) { return !std::isspace(ch); }).base(), str.end());

    return str;
}

std::string _LA_tokenize(std::string input, char delimiter, std::string *remaining)
{
    std::string token;
    size_t pos = input.find(delimiter);
    if (pos != std::string::npos) {
        token = input.substr(0, pos);
        if (remaining != nullptr) {
            *remaining = input.substr(pos + 1);
        }
    } else {
        token = input;
        if (remaining != nullptr) {
            *remaining = "";
        }
    }
    return token;
}

int _LA_ignorantStoi(std::string input, int defaultValue)
{
    try {
        return std::stoi(input);
    } catch (const std::invalid_argument& e) {
    } catch (const std::out_of_range& e) {
    }
    return defaultValue;
}



void LedAnimation::processCommand(std::string input)
{
	std::transform(
		input.begin(),
		input.end(),
		input.begin(),
		[](unsigned char c) { return std::tolower(c); }
	);

	std::string args;
	std::string command = _LA_tokenize(_LA_trim(input), ',', &args);

	int success = 0;
	if (command == "off"){
		this->SetMode(0);
		success = 1;
	}
	else if(command == "on"){
		this->SetMode(1);
		success = 1;
	}
	else if(command == "setlrgb" || command == "setlrgbww"){
		std::string level = _LA_tokenize(_LA_trim(args), ',', &args);
		if ( level != "" ){
			this->SetLevel(_LA_ignorantStoi(level, 100)); //level is command for global level
		}
		LedStripState state = LedStripState(
			100,
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args),0), //r
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args),0), //g
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args),0), //b
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args),0), //cw
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args),0)  //cc
		);
		this->UpdateMode1(state);
		success = 1;
	}
	else if(command == "chlevel"){
		this->ChangeLevel(_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args), 0));
		success = 1;
	}
	else if(command == "transitiontime"){
		this->default_transition_time = _LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args), 600);
		success = 1;
	}
	else if(command == "mode"){
		this->SetMode(_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args), 1));
		success = 1;
	}
	else if(command == "setmode"){
		this->UpdatePresetDefinition(
			_LA_ignorantStoi(_LA_tokenize(_LA_trim(args), ',', &args), 0) - LEDANIMATION_PRESETS_TO_MODE_OFFSET,
			args
		);
		success = 1;
	}
	else {
		//?
	}
}