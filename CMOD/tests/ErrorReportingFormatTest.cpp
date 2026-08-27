#include "CmodError.h"

#include <iostream>
#include <sstream>

static bool check(CmodError::Kind kind, const char* category, int exitCode) {
    CmodError error(kind, "Failure reason", "Inner context", "Corrective action");
    error.addContext("Outer context");
    std::ostringstream output;
    error.report(output, "Example.dissco");
    const std::string expected = std::string("CMOD ") + category + " error: Failure reason\n"
        "Project: Example.dissco\n"
        "Context: Outer context -> Inner context\n"
        "Suggestion: Corrective action\n"
        "Build failed.\n";
    if (output.str() != expected || error.exitCode() != exitCode) {
        std::cerr << "Incorrect " << category << " diagnostic:\n" << output.str();
        return false;
    }
    return true;
}

int main() {
    return check(CmodError::Kind::Project, "project", 1)
        && check(CmodError::Kind::Output, "output", 1)
        && check(CmodError::Kind::Internal, "internal", 2) ? 0 : 1;
}
