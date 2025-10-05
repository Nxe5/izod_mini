#pragma once
#include <Arduino.h>

bool wavStartFirstUnderMusic();
bool wavStartFile(const char* path);
void wavStop();
bool wavIsPlaying();
