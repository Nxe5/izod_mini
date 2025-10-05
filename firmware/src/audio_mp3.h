#pragma once
#include <Arduino.h>

bool mp3StartFirstUnderMusic();
bool mp3StartFile(const char* path);
void mp3Stop();
bool mp3IsPlaying();
void mp3EnsureTask();
