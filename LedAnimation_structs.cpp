#include "LedAnimation_structs.h"


LedStripState::LedStripState(){

}
LedStripState::LedStripState(unsigned char aLevel, unsigned char aR, unsigned char aG, unsigned char aB, unsigned char aWW, unsigned char aCW){
    level = aLevel;
    r = aR;
    g = aG;
    b = aB;
    ww = aWW;
    cw = aCW;
};

inline unsigned char LedStripState::applyLevel(unsigned char value, unsigned char globalLevel) const {
    return (int)value*level*globalLevel/10000;
}

unsigned char LedStripState::rLeveled(unsigned char globalLevel) const { return applyLevel(r, globalLevel); }
unsigned char LedStripState::gLeveled(unsigned char globalLevel) const { return applyLevel(g, globalLevel); }
unsigned char LedStripState::bLeveled(unsigned char globalLevel) const { return applyLevel(b, globalLevel); }
unsigned char LedStripState::wwLeveled(unsigned char globalLevel) const { return applyLevel(ww, globalLevel); }
unsigned char LedStripState::cwLeveled(unsigned char globalLevel) const { return applyLevel(cw, globalLevel); }

/*
#ifndef BUILD_NO_DEBUG
String LedStripState::dump(){
    String s = "State:";
    s += " L="; s += level;
    s += " R="; s += r;
    s += " G="; s += g;
    s += " B="; s += b;
    s += " WW="; s += ww;
    s += " CW="; s += cw;
    return s;
}
#endif*/

LedStripTransition::LedStripTransition(){
    state = LedStripState();
    transitionTime = 0;
}
LedStripTransition::LedStripTransition(LedStripState aState, int aTransitionTime){
    state = aState;
    transitionTime = aTransitionTime;
}

/*
#ifndef BUILD_NO_DEBUG
String LedStripTransition::dump(){
    String s = "Step:";
    s += " t="; s += transitionTime;
    s += " State="; s += state.dump();
    return s;
}
#endif*/


LedStripTransitionState::LedStripTransitionState(){
}
LedStripTransitionState::LedStripTransitionState(LedStripTransition aTransitionStep, int aStartTime){
    transitionStep = aTransitionStep;
    startTime = aStartTime;
    active = false;
}
unsigned char LedStripTransitionState::linear(unsigned char from, unsigned char to, float coef) const{
    return (int)(coef*(to-from) + from + 0.5f);
}
unsigned char LedStripTransitionState::easyInOut(unsigned char from, unsigned char to, float coef) const{
    float sqt = coef*coef;
    return linear(from, to, sqt / (2.0f * (sqt-coef) + 1.0f) );
}
void LedStripTransitionState::updateState(float coef, LedStripState *output)
{
    unsigned char (LedStripTransitionState::*transFunc)(unsigned char, unsigned char, float) const;
    transFunc = &LedStripTransitionState::easyInOut;

    output->level = (this->*transFunc)(startState.level, transitionStep.state.level, coef);
    output->r  	  = (this->*transFunc)(startState.r,     transitionStep.state.r,     coef);
    output->g     = (this->*transFunc)(startState.g,     transitionStep.state.g,     coef);
    output->b     = (this->*transFunc)(startState.b,     transitionStep.state.b,     coef);
    output->cw    = (this->*transFunc)(startState.cw,    transitionStep.state.cw,    coef);
    output->ww    = (this->*transFunc)(startState.ww,    transitionStep.state.ww,    coef);
}


LedStripAnimationPreset::LedStripAnimationPreset(LedStripTransition *aSteps, int aCount, bool aAllocated){
    allocated = aAllocated;
    steps = aSteps;
    countOfSteps = aCount;
}



LedStripAnimationState::LedStripAnimationState(LedStripAnimationPreset *aPreset)
{
    preset = aPreset;
    active = false;
    currentStep = 0;
}




//region static predefined presets

//As steps:
LedStripTransition __const_predefined_animation_steps_rainbow1[] = {
	LedStripTransition(LedStripState(100, 255, 0,   0  ,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 0,   255, 0  ,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 0,   0,   255,  0,  0), 3000),
};
LedStripTransition __const_predefined_animation_steps_rainbow2[] = {
	LedStripTransition(LedStripState(100, 255, 0,   0  ,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 255, 255, 0  ,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 0,   255, 0  ,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 0,   255, 255,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 0,   0,   255,  0,  0), 3000),
	LedStripTransition(LedStripState(100, 255, 0,   255,  0,  0), 3000),
};
LedStripTransition __const_predefined_animation_steps_strobo1[] = {
	LedStripTransition(LedStripState(100, 255, 255, 255, 255, 255), 40),
	LedStripTransition(LedStripState(100, 255, 255, 255, 255, 255), 160),
	LedStripTransition(LedStripState(100, 0,   0,   0,   0,   0), 40),
	LedStripTransition(LedStripState(100, 0,   0,   0,   0,   0), 760)
};
LedStripTransition __const_predefined_animation_steps_police1[] = {
	LedStripTransition(LedStripState(100, 0, 0, 255,  0,  0), 160),
	LedStripTransition(LedStripState(100, 255, 0, 0,  0,  0), 160),
};
LedStripTransition __const_predefined_animation_steps_feo1[] = {
	LedStripTransition(LedStripState(100, 255, 0,  66,  0,  0), 300),
	LedStripTransition(LedStripState( 50, 219, 11, 91,  0,  0), 200),
	LedStripTransition(LedStripState(100, 255, 0,  66,  0,  0), 200),
	LedStripTransition(LedStripState( 75, 219, 11, 91,  0,  0), 600),
	LedStripTransition(LedStripState( 75, 219, 11, 91,  0,  0), 6900),
};
//wrap to presets:
#define LEDANIMATION_STEPS_TO_PRESET(steps) LedStripAnimationPreset(steps, sizeof(steps)/sizeof(LedStripTransition), false)
LedStripAnimationPreset const_predefined_animation_rainbow1 = LEDANIMATION_STEPS_TO_PRESET(__const_predefined_animation_steps_rainbow1);
LedStripAnimationPreset const_predefined_animation_rainbow2 = LEDANIMATION_STEPS_TO_PRESET(__const_predefined_animation_steps_rainbow2);
LedStripAnimationPreset const_predefined_animation_strobo1 = LEDANIMATION_STEPS_TO_PRESET(__const_predefined_animation_steps_strobo1);
LedStripAnimationPreset const_predefined_animation_police1 = LEDANIMATION_STEPS_TO_PRESET(__const_predefined_animation_steps_police1);
LedStripAnimationPreset const_predefined_animation_feo1 = LEDANIMATION_STEPS_TO_PRESET(__const_predefined_animation_steps_feo1);

//endregion


