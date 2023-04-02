#ifndef LEDSTRIP_DATA_STRUCT_H
#define LEDSTRIP_DATA_STRUCT_H


#include <stdint.h>

/**
 * @brief Holds state of colors and level, adds some usefull operations
 * 
 */
struct LedStripState {
	int16_t level = 255, r = 0, g = 0, b = 0, ww = 0, cw = 0;

    LedStripState();
	LedStripState(unsigned char aLevel, unsigned char aR, unsigned char aG, unsigned char aB, unsigned char aWW, unsigned char aCW);

    inline unsigned char applyLevel(unsigned char value, unsigned char globalLevel) const;
	unsigned char rLeveled(unsigned char globalLevel) const;
	unsigned char gLeveled(unsigned char globalLevel) const;
	unsigned char bLeveled(unsigned char globalLevel) const;
	unsigned char wwLeveled(unsigned char globalLevel) const;
	unsigned char cwLeveled(unsigned char globalLevel) const;
};
/**
 * @brief holds data for transition target 
 * 
 */
struct LedStripTransition {
	LedStripState state;
	int			  transitionTime;

    LedStripTransition();
	LedStripTransition(LedStripState aState, int aTransitionTime);
};
/**
 * @brief holds data of transition progress
 * 
 */
struct LedStripTransitionState {
	LedStripTransition transitionStep;
	bool		  active;
	LedStripState startState;
	unsigned long startTime;//system ms

    LedStripTransitionState();
	LedStripTransitionState(LedStripTransition aTransitionStep, int aStartTime);
	unsigned char linear(unsigned char from, unsigned char to, float coef) const;
	unsigned char easyInOut(unsigned char from, unsigned char to, float coef) const;
	void updateState(float coef, LedStripState *output);
};
/**
 * @brief set of steps for animations
 * 
 */
struct LedStripAnimationPreset {
	bool allocated;
	unsigned char countOfSteps;
	LedStripTransition *steps;

	LedStripAnimationPreset(LedStripTransition *aSteps, int aCount, bool aAllocated);
};

struct LedStripAnimationState {
	LedStripAnimationPreset *preset;
	bool				    active;
	int						currentStep;

	LedStripAnimationState(LedStripAnimationPreset *aPreset);
};


//region static predefined presets

extern LedStripAnimationPreset const_predefined_animation_rainbow1;
extern LedStripAnimationPreset const_predefined_animation_rainbow2;
extern LedStripAnimationPreset const_predefined_animation_strobo1;
extern LedStripAnimationPreset const_predefined_animation_police1;
extern LedStripAnimationPreset const_predefined_animation_feo1;

//endregion



#endif // ifndef LEDSTRIP_DATA_STRUCT_H