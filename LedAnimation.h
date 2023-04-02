#ifndef LEDANIMATION_H
#define LEDANIMATION_H

#include <string>
#include <vector>

#include <algorithm>
#include <cctype>

#include "LedAnimation_structs.h"
#include "DefinitionParser.h"

//index difference between mode idx and presets idx
#define LEDANIMATION_PRESETS_COUNT (8)
#define LEDANIMATION_PRESETS_TO_MODE_OFFSET  (2)

class LedAnimation {
    public:
        //this is used on transition of changes in mode 1
        unsigned long default_transition_time 		     = 600;

        /**
         * Update animation. Frequency of calls must be at least 20x/sec for fluent animation, 50x is safer
         * Return values to apply to PWM outputs. Global level already applied, so use only r,g,b,cc,wc
        */
        int loopTick(LedStripState *output);

        //ptr to method returning current absolute ms time
        void setTimeCallback(unsigned long (*callback)(void));
        
        void SetMode(int newMode);//0 off, 1 on (no animation), 2+ == presets
        void SetLevel(int newValue);
        void ChangeLevel(int relativeValue);

        void UpdateMode1(LedStripState newState);
        void UpdatePresetDefinition(int presetId, std::string definitionString);
        void UpdatePresetDefinition(int presetId, std::vector<LedStripTransition> steps);
        void processCommand(std::string command);

    protected:
        //real state on putput
        LedStripState   current_state_ledStrip			     = LedStripState(100,0,0,0,0,0);
        //target color in normal mode
        LedStripState   mode1_state_ledStrip		     	 = LedStripState(100,0,0,0,0,0);

        LedStripAnimationPreset *presets[LEDANIMATION_PRESETS_COUNT] = {
            &const_predefined_animation_rainbow1, //mode 2
            &const_predefined_animation_rainbow2, //mode 3
            &const_predefined_animation_strobo1,
            &const_predefined_animation_police1,
            &const_predefined_animation_feo1,
            nullptr,//7
            nullptr,//8
            nullptr//9
        };

        LedStripTransitionState transition_state      = LedStripTransitionState(LedStripTransition(LedStripState(100,0,0,0,0,0), 0), 0);
        LedStripAnimationState animation_state        = LedStripAnimationState(nullptr);

        unsigned char current_mode;

        unsigned char current_level;
        unsigned char target_level;

        unsigned long (*timeCallback)(void) = nullptr;

        bool AnimationStart(int presetId);
        void AnimationNextStep(unsigned long startTime = 0);
        void StartTransition(LedStripTransition transitionStep, unsigned long startTime = 0);
        unsigned long GetCurrentTime();
};

#endif // ifndef LEDANIMATION_H