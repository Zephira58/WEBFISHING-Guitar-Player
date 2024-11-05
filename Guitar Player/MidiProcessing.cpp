#include "MidiProcessing.h"

// Function to calculate the optimal shift for notes
int calculateOptimalShift(const std::vector<uint8_t>& notes) {
    ShiftParameters parameters;
    int bestShift = 0;
    double bestScore = -std::numeric_limits<double>::max();

    for (int shift = -127; shift <= 127; shift++) {
        int playableNotes = 0;
        for (uint8_t note : notes) {
            if (note + shift >= 40 && note + shift <= 79) {
                playableNotes++;
            }
        }

        // Calculate normalized score
        double normalizedPlayableScore = static_cast<double>(playableNotes) / notes.size();
        double score = normalizedPlayableScore * parameters.playableNoteWeight;

        // Apply bonuses for preferred shifts
        if (shift == 0) {
            score += parameters.noShiftBonus;
        }
        else if (shift % 12 == 0) {
            score += parameters.octaveShiftBonus;
        }

        // Penalize larger shifts
        score -= std::abs(shift) * parameters.maxShiftPenalty / 127.0;

        if (score > bestScore || (score == bestScore && std::abs(shift) < std::abs(bestShift))) {
            bestScore = score;
            bestShift = shift;
        }
    }

    return bestShift;
}

std::string convertMidiToText(const std::filesystem::path& midiFilePath) {
    smf::MidiFile midifile;
    if (!midifile.read(midiFilePath.string())) {
        throw std::runtime_error("Error reading MIDI file: " + midiFilePath.string());
    }

    midifile.doTimeAnalysis();
    midifile.linkNotePairs();

    std::vector<SoundEvent> song;
    std::vector<uint8_t> allNotes;

    // Process MIDI events
    for (int track = 0; track < midifile.getTrackCount(); track++) {
        for (int event = 0; event < midifile[track].size(); event++) {
            if (midifile[track][event].isNoteOn()) {
                int timestamp = static_cast<int>(midifile[track][event].seconds * 1000);
                uint8_t noteNumber = midifile[track][event][1];

                if (!song.empty() && timestamp <= song.back().timestamp + 30) {
                    song.back().notes.push_back(noteNumber);
                }
                else {
                    song.emplace_back(timestamp, noteNumber);
                }
                allNotes.push_back(noteNumber);
            }
        }
    }

    if (song.empty()) {
        throw std::runtime_error("No notes found in MIDI file: " + midiFilePath.string());
    }

    // Calculate optimal shift
    int optimalShift = calculateOptimalShift(allNotes);

    // Build the output string
    std::stringstream output;
    int offsetNotes = song[0].timestamp;

    for (const auto& event : song) {
        std::vector<int> shiftedNotes;
        for (uint8_t note : event.notes) {
            int shiftedNote = note + optimalShift;
            if (shiftedNote >= 40 && shiftedNote <= 79) {
                shiftedNotes.push_back(shiftedNote);
            }
        }

        if (!shiftedNotes.empty()) {
            // Sort notes in ascending order
            std::sort(shiftedNotes.begin(), shiftedNotes.end());

            output << (event.timestamp - offsetNotes);
            for (int note : shiftedNotes) {
                output << " " << note;
            }
            output << "\n";
        }
    }

    return output.str();
}
