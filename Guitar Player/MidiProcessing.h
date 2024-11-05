#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "Midifile/MidiFile.h"


struct SoundEvent {
    int timestamp;
    std::vector<uint8_t> notes;

    SoundEvent(int time, uint8_t note) : timestamp(time) {
        notes.push_back(note);
    }

    SoundEvent(int time, const std::vector<uint8_t>& tempNotes)
        : timestamp(time), notes(tempNotes) {
    }
};

struct ShiftParameters {
    double noShiftBonus = 0.11;
    double octaveShiftBonus = 0.15;
    double maxShiftPenalty = 0.3;
    double playableNoteWeight = 2.2;
};

int calculateOptimalShift(const std::vector<uint8_t>& notes);
std::string convertMidiToText(const std::filesystem::path& midiFilePath);