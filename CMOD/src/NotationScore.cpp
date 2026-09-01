#include "NotationScore.h"
#include "CmodError.h"

NotationScore::NotationScore() : 
    score_title_("Score"),
    is_built_(false) {}

// NotationScore::NotationScore(const string& score_title) :
//     is_built_(false),
//     score_title_(score_title) {
//     }

// multistaffs
NotationScore::NotationScore(const string& score_title,bool grandStaff, int numberOfStaff) :
    score_title_(score_title),
    is_built_(false) {
    // initialize the staff
    if(grandStaff){
      is_grand = true;
      staffSum = 2;
    }else{
      is_grand = false;
      if(numberOfStaff > 0){
        staffSum = numberOfStaff;
      }else{
        cout<<"WARNING: the total number of staffs should be an integer and greater than 0." << "\n";
        staffSum = 1;
      } 
    }
    // define the layer number for the staff layer.
    score_staff.resize(staffSum);
    }

// void NotationScore::RegisterTempo(Tempo& tempo) {
//   // Find insertion point by comparing the global start in __seconds__
//   TimeSignature ts = TimeSignature(tempo);
//   vector<Section>::iterator section_iter = score_.begin();
//   while (section_iter != score_.end() && 
//          *section_iter < ts) {
//     ++section_iter;
//   }

//   if (score_.empty() || *section_iter != ts) {
//     score_.insert(section_iter, Section(ts));
//   }
// }

// multistaffs
void NotationScore::RegisterTempo(Tempo& tempo,int staffNum) {
  // make sure staffNum is a valid number
  // start from 0
  if(staffNum < 0){
    staffNum = 0;
  }
  if(staffNum >= (int)score_staff.size()){
    staffNum = staffSum-1;
  }
  // Find insertion point by comparing the global start in __seconds__
  TimeSignature ts = TimeSignature(tempo);
  vector<Section>::iterator section_iter = score_staff[staffNum].begin();
  while (section_iter != score_staff[staffNum].end() && 
         *section_iter < ts) {
    ++section_iter;
  }

  if (section_iter == score_staff[staffNum].end() || *section_iter != ts) {
    score_staff[staffNum].insert(section_iter, Section(ts));
  }
}

// void NotationScore::InsertNote(Note* n) {
//   if (score_.empty()) {
//     cerr << "Cannot add note to score without any sections!" << endl;
//     exit(1);
//   }

//   vector<Section>::iterator section_iter = score_.begin();
//   while (section_iter != score_.end() && !(*section_iter).InsertNote(n)) ++section_iter;

//   if (section_iter == score_.end()) {
//     cerr << "Note does not belong to any section in the score!" << endl;
//     exit(1);
//   }
// }

// multistaffs
void NotationScore::InsertNote(Note* n) {
  // judge if the staff number is out of range
  if(n->getStaffNum()>=staffSum){
    n->setStaffNum(staffSum-1);
  }
  if (score_staff[n->getStaffNum()].empty()) {
    throw CmodError(CmodError::Kind::Internal,
                    "A note was generated before its score tempo was registered.",
                    "Score output, staff " + to_string(n->getStaffNum()),
                    "Report this error to the DISSCO developers with the project file, seed, and full output.");
  }

  vector<Section>::iterator section_iter = score_staff[n->getStaffNum()].begin();
  while (section_iter != score_staff[n->getStaffNum()].end() && !(*section_iter).InsertNote(n)) ++section_iter;

  if (section_iter == score_staff[n->getStaffNum()].end()) {
    throw CmodError(CmodError::Kind::Internal,
                    "A note could not be assigned to a registered tempo section.",
                    "Score output, staff " + to_string(n->getStaffNum()),
                    "Report this error to the DISSCO developers with the project file, seed, and full output.");
  }
}

// void NotationScore::Build() {
//   if (!is_built_) {
//     // Since all tempos are registered, calculate their start times 
//     // in terms of the previous tempo's EDU's
//     vector<Section>::iterator iter = score_.begin();
//     vector<Section>::iterator next = score_.begin() + 1;
//     int last_start_time_edu = 0;

//     while (next != score_.end()) {
//       float dur_seconds = next->GetStartTimeGlobal() - iter->GetStartTimeGlobal();
//       iter->SetDurationEDUS(iter->CalculateEDUsFromSecondsInTempo(dur_seconds));
//       iter->Build(true);
//       ++iter; ++next;
//     }
//     iter->SetDurationEDUS(-1);
//     iter->Build(true);

//     is_built_ = true;
//   }
// }

// multistaffs

void NotationScore::Build() {
  if (!is_built_) {
    for(int i=0 ; i<staffSum; i++){
      try {
      if (score_staff[i].empty()) {
        throw CmodError(CmodError::Kind::Project,
                        "No note events were generated for staff " + to_string(i) + ".",
                        "Score output",
                        "Add note events assigned to this staff, correct the staff settings, or disable score output.");
      }
      // Since all tempos are registered, calculate their start times 
      // in terms of the previous tempo's EDU's
      vector<Section>::iterator iter = score_staff[i].begin();
      vector<Section>::iterator next = score_staff[i].begin() + 1;
      string previous_time_signature;
      bool first_section = true;    

      while (next != score_staff[i].end()) {
        float dur_seconds = next->GetStartTimeGlobal() - iter->GetStartTimeGlobal();
        iter->SetDurationEDUS(iter->CalculateEDUsFromSecondsInTempo(dur_seconds));

        string current_time_signature = iter->GetTimeSignature().time_signature_;
        bool print_time_signature = first_section || current_time_signature != previous_time_signature;
        iter->Build(print_time_signature);

        first_section = false;
        previous_time_signature = current_time_signature;
        ++iter; ++next;
      }

      iter->SetDurationEDUS(-1);
      string current_time_signature = iter->GetTimeSignature().time_signature_;
      bool print_time_signature = first_section || current_time_signature != previous_time_signature;
      iter->Build(print_time_signature);
      } catch (CmodError& error) {
        error.addContext("Score output, staff " + to_string(i));
        throw;
      }
    }
    is_built_ = true;
  }
}

// ostream& operator<<(ostream& output_stream,
//                     NotationScore& notation_score) {
//   if (!notation_score.is_built_) {
//     notation_score.Build();
//   }

//   output_stream << "\\header {\n  title=\"" << notation_score.score_title_ 
//                 << "\"\ncomposer=\"DISSCO\"\n}" << endl;
//   output_stream << "\\new Voice \\with {" << endl;
//   output_stream << "\\remove \"Note_heads_engraver\"" << endl;
//   output_stream << "\\consists \"Completion_heads_engraver\"" << endl;
//   output_stream << "\\remove \"Rest_engraver\"" << endl;
//   output_stream << "\\consists \"Completion_rest_engraver\"" << endl;
//   output_stream << "}" << endl;

//   output_stream << "{" << endl;
//   // Staffs
//   // TimeSignature
//   for (vector<Section>::iterator iter = notation_score.score_.begin();
//        iter != notation_score.score_.end();
//        ++iter) {
//     iter->PrintAllNotesFlat("Final output");
//     list<Note*> section_flat = iter->GetSectionFlat();
//     // for each notes
//     for (list<Note*>::iterator iter_iter = section_flat.begin();
//          iter_iter != section_flat.end();
//          ++iter_iter) {
//       Note*& cur_note = *iter_iter;
//       output_stream << cur_note->GetText();
//     }
    
//     output_stream << '\n';
//   }
  
//   output_stream << "\\bar \"|.\"" << endl;
//   output_stream << "}" << endl;

//   return output_stream;
// }

// multistaffs
ostream& operator<<(ostream& output_stream,
                    NotationScore& notation_score) {
  if (!notation_score.is_built_) {
    notation_score.Build();
  }

  output_stream << "\\header {\n  title=\"" << notation_score.score_title_ 
                << "\"\ncomposer=\"DISSCO\"\n}" << endl;
  output_stream << "\\version \"2.18.2\" " << endl;
  // output_stream << "\\new Voice \\with {" << endl;
  // output_stream << "\\remove \"Note_heads_engraver\"" << endl;
  // output_stream << "\\consists \"Completion_heads_engraver\"" << endl;
  // output_stream << "\\remove \"Rest_engraver\"" << endl;
  // output_stream << "\\consists \"Completion_rest_engraver\"" << endl;
  // output_stream << "}" << endl;

  // Staffs
  if(notation_score.is_grand){
    output_stream << "\\new GrandStaff " << endl;
  }
  output_stream << " << " << endl;
  cout << "staffSum:" << notation_score.staffSum << endl;
  for(int i=0;i<notation_score.staffSum; i++){
    output_stream << "\\new Staff" << endl;
    output_stream << "{" << endl;
    // TimeSignature
    if(!notation_score.score_staff[i].empty()){
      for (vector<Section>::iterator iter = notation_score.score_staff[i].begin();
        iter != notation_score.score_staff[i].end();
        ++iter) {
        iter->PrintAllNotesFlat("Final output");
        list<Note*> section_flat = iter->GetSectionFlat();
        // for each notes
        string notes_stream=" ";
        // use to decide treble or bass
        int avePitchNum=0;
        int pitchSum=0;
        for (list<Note*>::iterator iter_iter = section_flat.begin();
            iter_iter != section_flat.end();
            ++iter_iter) {
          Note*& cur_note = *iter_iter;
          notes_stream = notes_stream + cur_note->GetText();
          if(cur_note->is_real_note()){
            avePitchNum = avePitchNum + cur_note->getPitchNum();
            pitchSum = pitchSum + 1;
          }
        }
        if (pitchSum == 0) {
          throw CmodError(CmodError::Kind::Project,
                          "No notes with a notatable duration remain in this score section.",
                          "Score output, staff " + to_string(i) + ", section starting at " +
                              to_string(iter->GetStartTimeGlobal()) + " seconds",
                          "Check note durations and EDU settings. Give notes a positive duration that can be represented in the score, or disable score output.");
        }
        avePitchNum = avePitchNum / pitchSum;
        // if the average pitch number isn't smaller than 48, choose treble
        // else choose bass
        if(avePitchNum >= 48){
          output_stream << "\\clef treble" << endl;
        }else{
          output_stream << "\\clef bass" << endl;
        }
        output_stream << notes_stream;
      }
      output_stream << "\\bar \"|.\"";
    }
    output_stream << "}" << endl;
  }

  // if(notation_score.is_grand){
  //   output_stream << ">>" << endl;
  // }
  output_stream << ">>" << endl;

  return output_stream;
}
