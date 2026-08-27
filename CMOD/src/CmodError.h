#ifndef CMOD_ERROR_H
#define CMOD_ERROR_H

#include <ostream>
#include <stdexcept>
#include <string>

// Expected failures retain the input context and a useful next step while
// unwinding to the command-line boundary. Unexpected exceptions are internal.
class CmodError : public std::runtime_error {
public:
    enum class Kind { Project, Output, Internal };

    CmodError(Kind kind, const std::string& message,
              const std::string& context, const std::string& suggestion)
        : std::runtime_error(message), kind_(kind), context_(context),
          suggestion_(suggestion) {}

    void addContext(const std::string& context) {
        context_ = context + (context_.empty() ? "" : " -> " + context_);
    }

    int exitCode() const { return kind_ == Kind::Internal ? 2 : 1; }

    void report(std::ostream& output, const std::string& project) const {
        const char* category = kind_ == Kind::Project ? "project"
                             : kind_ == Kind::Output ? "output" : "internal";
        output << "CMOD " << category << " error: " << what() << '\n'
               << "Project: " << project << '\n';
        if (!context_.empty())
            output << "Context: " << context_ << '\n';
        output << "Suggestion: " << suggestion_ << '\n'
               << "Build failed." << std::endl;
    }

private:
    Kind kind_;
    std::string context_;
    std::string suggestion_;
};

#endif
