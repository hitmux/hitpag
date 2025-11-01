#include "target_conflict.h"

#include <algorithm>
#include <cctype>

namespace target_conflict {

namespace {
char to_lower(char ch) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}
} // namespace

Action prompt_action(
    const std::function<void(const std::string&)>& output_fn,
    const std::function<std::string()>& input_fn,
    const std::string& header,
    const std::string& options_line,
    const std::string& prompt_line,
    const std::string& invalid_choice_line) {

    output_fn(header + "\n");
    output_fn(options_line + "\n");

    while (true) {
        output_fn(prompt_line);
        std::string choice = input_fn();
        if (!choice.empty()) {
            char ch = to_lower(choice.front());
            if (ch == 'o') return Action::Overwrite;
            if (ch == 'c') return Action::Cancel;
            if (ch == 'r') return Action::Rename;
        }
        output_fn(invalid_choice_line + "\n");
    }
}

std::string prompt_new_path(
    const std::function<void(const std::string&)>& output_fn,
    const std::function<std::string()>& input_fn,
    const std::string& prompt_line,
    const std::string& default_value) {
    output_fn(prompt_line);
    std::string value = input_fn();
    if (value.empty()) {
        return default_value;
    }
    return value;
}

} // namespace target_conflict
