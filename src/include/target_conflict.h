#pragma once

#include <functional>
#include <string>

namespace target_conflict {

/**
 * Defines the available actions when a target path already exists.
 */
enum class Action {
    Overwrite,
    Cancel,
    Rename
};

/**
 * Presents a choice to the user and parses the response.
 *
 * The function delegates input/output handling to the provided callbacks so it can be used
 * in both interactive mode and non-interactive command-line mode.
 *
 * @param output_fn Function used to display messages to the user.
 * @param input_fn Function used to collect user input (should return a trimmed string).
 * @param header Message shown before the options list (e.g., informing that the target exists).
 * @param options_line Describes the available options (e.g., "[O]verwrite / [C]ancel / [R]ename").
 * @param prompt_line Prompt displayed when waiting for the user's choice.
 * @param invalid_choice_line Message displayed when the input cannot be parsed.
 * @return The parsed user action.
 */
Action prompt_action(
    const std::function<void(const std::string&)>& output_fn,
    const std::function<std::string()>& input_fn,
    const std::string& header,
    const std::string& options_line,
    const std::string& prompt_line,
    const std::string& invalid_choice_line);

/**
 * Prompts the user for a new target path when they choose to rename.
 *
 * @param output_fn Function used to display the prompt.
 * @param input_fn Function used to collect user input (should return a trimmed string).
 * @param prompt_line Prompt displayed to request the new path.
 * @return The user supplied path.
 */
std::string prompt_new_path(
    const std::function<void(const std::string&)>& output_fn,
    const std::function<std::string()>& input_fn,
    const std::string& prompt_line,
    const std::string& default_value);

} // namespace target_conflict
