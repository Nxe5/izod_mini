#pragma once
#include <Arduino.h>

bool audioInit();
void audioSetPlaying(bool play);
bool audioIsPlaying();
void audioSetVolume(int percent);
int audioGetVolume();


