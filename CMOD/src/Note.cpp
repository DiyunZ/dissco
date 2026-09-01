/*
CMOD (composition module)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

//----------------------------------------------------------------------------//
//
//   Note.cpp
//
//----------------------------------------------------------------------------//

#include "Note.h"
#include "CmodError.h"
#include "Event.h"
#include "Output.h"
#include "Rational.h"
#include "tables.h"
#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cmath>

using namespace std;

//----------------------------------------------------------------------------//

Note::Note(TimeSpan ts, const Event* root_exact_ancestor)
    : ts(ts),
      rootExactAncestor(root_exact_ancestor),
      pitchNum(0),
      octaveNum(0),
      octavePitch(0),
      loudnessNum(0),
      start_t(0),
      end_t(0),
      tuplet(0),
      split(0),
      staffNum(0),
      type(NoteType::kUnknown),
      first_notation_fragment(true) {
}

//----------------------------------------------------------------------------//

Note::Note()
    : rootExactAncestor(nullptr),
      pitchNum(0),
      octaveNum(0),
      octavePitch(0),
      loudnessNum(0),
      start_t(0),
      end_t(0),
      tuplet(0),
      split(0),
      staffNum(0),
      type(NoteType::kUnknown),
      first_notation_fragment(true) {
}

//----------------------------------------------------------------------------//

Note::Note(const Note& other) = default;

//----------------------------------------------------------------------------//

bool Note::operator < (const Note& rhs) {
  //Sort notes by their *global* start time.
  return (ts.start < rhs.ts.start);
}

//----------------------------------------------------------------------------//

void Note::setStartTime(int start_time) {
  start_t = start_time;
}

//----------------------------------------------------------------------------//

void Note::setEndTime(int end_time) {
  end_t = end_time;
}
//----------------------------------------------------------------------------//
void Note::initSplit(){
  split = 0;
}
//----------------------------------------------------------------------------//

void Note::setPitchWellTempered(int absPitchNum) {
  // store the pitchNum in Note
  setPitchNum(absPitchNum);
  pitchNum = absPitchNum;
  octaveNum = pitchNum / 12;

  octavePitch = pitchNum % 12;
  if (octavePitch < 0) {
    octavePitch += 12;
    --octaveNum;
  }
  pitchName = pitchNames[octavePitch];

  string pitch = OutNames[octavePitch];
  string sign = octaveNum < 3 ? string(3 - octaveNum, ',')
                             : string(octaveNum - 3, '\'');
  chord_tones.clear();
  chord_tones.push_back({pitch + sign, modifiers, INT_MIN});
  rebuildPitchOutput();

  
  Output::addProperty("Pitch Number", pitchNum, "semitones");
  Output::addProperty("Pitch Name", pitchName);
  Output::addProperty("Octave Number", octaveNum);
  Output::addProperty("Pitch In Octave", octavePitch);
}

//----------------------------------------------------------------------------//

int Note::HertzToPitch(float freqHz) {

  if ( freqHz >= CEILING || freqHz <= MINFREQ) {
    cerr << "Warning: Note Frequency is " << freqHz << " Hz, outside the nominal "
         << MINFREQ << " to " << CEILING << " Hz range; using the nearest tempered pitch. "
         << "Suggestion: Check the Bottom event's Frequency setting if this pitch is not intended." << endl;
  }

  const int nearestPitch = static_cast<int>(rint(12 * log2(freqHz / C0)));
  setPitchNum(nearestPitch);

  return nearestPitch;
}

//----------------------------------------------------------------------------//

void Note::setLoudnessMark(int dynamicNum, vector<string> dynamicNames) {
  loudnessNum = dynamicNum;
  loudnessMark = dynamicNames[loudnessNum];  // int dur[10] = {2, 4, 6, 8, 10, 16, 18, 32, 64, 80};
  // int note = 23;
  // int unit_note = 8;
  // string pitch = "c";
  // string output = "";
  // while(note > 0){
  //   int power = check_pow(unit_note);
  //   while(power >= 0){
  //     int beats = po(2, power);
  //     if(note >= beats){
  //       output += pitch + Output::int_to_str(unit_note/beats);
  //       note -= beats;
  //       if(note >= beats/2){
  //         output += ".";
  //         note -= beats/2;
  //       }
  //       output += " ";
  //       break;
  //     }
  //     power--;
  //   }
  // }
  // cout << output << endl;
  // return 0;
  Output::addProperty("Dynamic", loudnessMark);
  Output::addProperty("Dynamic Level", loudnessNum);
}

//----------------------------------------------------------------------------//

void Note::setLoudnessSones(float sones) {
  loudnessNum = -1;
  // cout << " sones: " << sones << endl;
  if(!std::isfinite(sones) || sones < 0 || sones > 256) {
    throw CmodError(CmodError::Kind::Project,
                    "Note loudness is " + to_string(sones) + " sones, outside the supported range.",
                    "Note loudness (sones)",
                    "Set Loudness to a finite value from 0 to 256 sones for note events.");
  } else if(sones <= 4) {
    loudnessMark = "ppp";
  } else if(sones <= 8) {
    loudnessMark = "pp";
  } else if(sones <= 16) {
    loudnessMark = "p";
  } else if(sones <= 32) {
    loudnessMark = "mp";
  } else if(sones <= 45) {
    loudnessMark = "mf";
  } else if(sones <= 64) {
    loudnessMark = "f";
  } else if(sones <= 128) {
    loudnessMark = "ff";
  } else if (sones <= 256) {
    loudnessMark = "fff";
  }

  // for particel
  Output::addProperty("Loudness", loudnessMark);
  loudness_out = char(92) + loudnessMark;

}


//----------------------------------------------------------------------------//

bool is_attach_mark(string mod_name){
    for (int i=0; i< 39; i++)
        if (mod_name == modifiers[i])
            return true;
    return false;
}

// bool is_attach_mark(string mod_name){
//     for (int i=0; i< 40; i++)
//         if (mod_name == modifiers[i])
//             return true;
//     return false;
// }

//----------------------------------------------------------------------------//

void Note::setModifiers(vector<string> modNames) {
  modifiers.clear();
  for(unsigned i = 0; i < modNames.size(); i++) {
    if (is_attach_mark(modNames[i])){
      modifiers.push_back("\\" + modNames[i]);
    }
  }

  if (chord_tones.size() == 1) {
    chord_tones.front().modifiers = modifiers;
  }
}
//----------------------------------------------------------------------------//

void Note::prepareForInsertion() {
  for (ChordTone& tone : chord_tones) {
    if (tone.attack_edu == INT_MIN) {
      tone.attack_edu = start_t;
    }
  }
  rebuildPitchOutput();
}

void Note::mergePitches(const Note& other, bool prepend_other) {
  if (prepend_other) {
    chord_tones.insert(
        chord_tones.begin(), other.chord_tones.begin(), other.chord_tones.end());
  } else {
    chord_tones.insert(
        chord_tones.end(), other.chord_tones.begin(), other.chord_tones.end());
  }
  rebuildPitchOutput();
}

void Note::beginNotation() {
  first_notation_fragment = true;
}

string Note::nextPitchOutput() {
  const string output = renderPitch(first_notation_fragment);
  first_notation_fragment = false;
  return output;
}

string Note::renderPitch(bool include_modifiers) const {
  if (chord_tones.empty()) {
    return pitch_out;
  }

  string output = "<";
  for (size_t tone_idx = 0; tone_idx < chord_tones.size(); ++tone_idx) {
    if (tone_idx > 0) {
      output += " ";
    }

    const ChordTone& tone = chord_tones[tone_idx];
    output += tone.pitch;
    if (include_modifiers && tone.attack_edu == start_t) {
      for (vector<string>::const_reverse_iterator modifier = tone.modifiers.rbegin();
           modifier != tone.modifiers.rend();
           ++modifier) {
        output += *modifier;
      }
    }
  }
  output += ">";
  return output;
}

void Note::rebuildPitchOutput() {
  if (!chord_tones.empty()) {
    pitch_out = renderPitch(false);
  }
}

void Note::adjustStartTime(int new_start_time) {
  for (ChordTone& tone : chord_tones) {
    if (tone.attack_edu == start_t) {
      tone.attack_edu = new_start_time;
    }
  }
  start_t = new_start_time;
}

void Note::shiftEDUs(int offset) {
  start_t += offset;
  end_t += offset;
  for (ChordTone& tone : chord_tones) {
    if (tone.attack_edu != INT_MIN) {
      tone.attack_edu += offset;
    }
  }
}

//----------------------------------------------------------------------------//

// multistaffs
void Note::setStaffNum(int noteStaff){
  if(noteStaff<0){
    noteStaff = 0;
  }
  staffNum = noteStaff;
}

int Note::getStaffNum(){
  return staffNum;
}

int Note::getPitchNum(){
  return pitchNum;
}

void Note::setPitchNum(int notePitchNum){
  pitchNum = notePitchNum;
}

bool Note::is_real_note(){
  if(type==NoteType::kNote){
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------//

const string& Note::GetText() const {
  return type_out;
}

//----------------------------------------------------------------------------//

int Note::str_to_int(string s) {
  int temp = 0;
  for (unsigned i = 0; i < s.length(); ++i) {
    int x = int(s[i] - 48);
    temp = temp*10 + x;
  }
  return temp;
}

//----------------------------------------------------------------------------//

string Note::int_to_str(int n) {
  stringstream ss;
  ss << n;
  return ss.str();
}

