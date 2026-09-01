/*
CMOD (composition module)
Copyright (C) 2005  Sever Tipei (s-tipei@uiuc.edu)
Modified by Ming-ching Chiu 2013


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
//  Main.cpp
//
//  This is the main program for CMOD, the computer-assisted
//  composition software. It creates the Piece object which reads the .dissco
//  file (in XML format).
//---------------------------------------------------------------------------//

/**
 * @file Main.cpp
 * @brief CMOD command-line entry point.
 *
 * Parses arguments, installs signal handlers, instantiates the @ref Piece
 * model from the supplied `.dissco` project file, and drives it through
 * parse, expand, and emit-output. All long-lived state is owned by the
 * Piece — Main.cpp is a thin orchestrator.
 *
 * Historical change log preserved from the original sources:
 *  - 1/29/07: Justin King added doxygen commenting
 *  - 10/29/12: Ming-ching Chiu revised the code to remove filevalue
 */

#include "Piece.h"
#include <time.h>
#include "Note.h"
#include "SignalHandlers.h"
#include "CmodError.h"

			//added by Sever must be a more elegant way
#include <filesystem>
#include <iostream>
#include <fstream>
using namespace std;

static int runCmod(int parameterCount, char **parameterList) {
  // Rubin Du 2024: Installed custom signal handler to print stack trace on segfault
  signal(SIGSEGV, segfaultHandler);

  time_t startTime;
  time(&startTime);

  //Determine settings.
  cout << endl;
  cout << "=========================SETTINGS==========================" << endl;

  //Determine the project path.
  string path;
  if(parameterCount >= 2)
    path = parameterList[1];
  if(path == "--help" || path == "-help" || path == "help") {
    cout << "Usage: cmod <project.dissco>   Builds the specified project." << endl;
    //cout << "       cmod <path> <process-offset=0> <process-count=1>" << endl;
    //cout << "                     Renders a specific mask of sounds." << endl;
    cout << "       cmod help   Displays this help." << endl;
    return 0;
  }

  if (path.empty()) {
    throw CmodError(CmodError::Kind::Project, "No project file was specified.",
                    "Command line", "Run cmod <path-to-project.dissco>.");
  }

    const filesystem::path projectPath(path);
    filesystem::path workingDirectory = projectPath.parent_path();
    if (workingDirectory.empty())
        workingDirectory = ".";

    string workingPath = workingDirectory.string();
  cout << "Working in path: " << workingPath << endl;

  //Determine the project name.
    string projectName = projectPath.stem().string();

  //Create the piece!
  Piece piece(workingPath, projectName);
  //delete outputFile;		//Sever

  time_t endTime;
  time(&endTime);

  int seconds = static_cast<int>(difftime(endTime, startTime));
  int hr = seconds / 3600;
  int min = (seconds % 3600) / 60;
  int sec = seconds % 60;
  printf("Computation Time: %02d:%02d:%02d.\n", hr, min, sec);


  return 0;

}

int main(int parameterCount, char **parameterList) {
  const char* project = parameterCount >= 2 ? parameterList[1] : "(not specified)";
  try {
    return runCmod(parameterCount, parameterList);
  } catch (const CmodError& error) {
    error.report(cerr, project);
    return error.exitCode();
  } catch (const std::exception& error) {
    CmodError failure(CmodError::Kind::Internal, error.what(), "Building project",
                      "Report this problem to the DISSCO developers with the project, seed, and this diagnostic.");
    failure.report(cerr, project);
    return failure.exitCode();
  } catch (...) {
    CmodError failure(CmodError::Kind::Internal, "An unexpected failure occurred.",
                      "Building project",
                      "Report this problem to the DISSCO developers with the project, seed, and this diagnostic.");
    failure.report(cerr, project);
    return failure.exitCode();
  }
}
