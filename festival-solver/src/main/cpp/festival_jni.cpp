#include <jni.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <exception>
#include <mutex>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "global.h"

int festival_cli_main(int argc, char **argv);

extern int forced_alg;
extern int patterns_num;
extern int pull_patterns_num;
extern int total_sol_time;
extern int total_levels_tried;
extern int total_levels_solved;
extern int global_total_pushes;
extern int global_total_moves;
extern std::atomic<int> festival_cancel_requested;

static std::mutex solver_mutex;

static int normalize_cores(int requested) {
    if (requested >= 8) return 8;
    if (requested >= 4) return 4;
    if (requested >= 2) return 2;
    return 1;
}

static std::string from_jstring(JNIEnv *env, jstring value) {
    if (value == nullptr) return "";
    const char *chars = env->GetStringUTFChars(value, nullptr);
    std::string out(chars == nullptr ? "" : chars);
    if (chars != nullptr) env->ReleaseStringUTFChars(value, chars);
    return out;
}

static bool write_text_file(const std::string &path, const std::string &text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << text;
    if (text.empty() || text.back() != '\n') out << '\n';
    return true;
}

static std::string read_text_file(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return "";
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static std::string trim_line(std::string line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    const size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    const size_t end = line.find_last_not_of(" \t");
    return line.substr(start, end - start + 1);
}

static bool is_solved_marker_line(std::string line) {
    line = trim_line(line);
    return line == "SOLVED!" || line.rfind("SOLVED! ", 0) == 0;
}

static bool contains_solved_marker(const std::string &text) {
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (is_solved_marker_line(line)) return true;
    }
    return false;
}

static bool is_lurd_line(const std::string &line) {
    if (line.empty()) return false;
    for (char c : line) {
        if (c != 'l' && c != 'u' && c != 'r' && c != 'd'
                && c != 'L' && c != 'U' && c != 'R' && c != 'D') {
            return false;
        }
    }
    return true;
}

static bool contains_solution_path(const std::string &text) {
    std::istringstream lines(text);
    std::string line;
    bool afterSolution = false;
    while (std::getline(lines, line)) {
        std::string s = trim_line(line);
        if (s == "Solution") {
            afterSolution = true;
            continue;
        }
        if (afterSolution) {
            if (is_lurd_line(s)) return true;
            if (!s.empty()) afterSolution = false;
        }
    }
    return false;
}

class StdioRedirect {
public:
    explicit StdioRedirect(const std::string &path) {
        out_dup_ = dup(STDOUT_FILENO);
        err_dup_ = dup(STDERR_FILENO);
        file_ = std::fopen(path.c_str(), "wb");
        if (file_ == nullptr) return;
        std::fflush(stdout);
        std::fflush(stderr);
        dup2(fileno(file_), STDOUT_FILENO);
        dup2(fileno(file_), STDERR_FILENO);
    }

    ~StdioRedirect() {
        restore();
    }

    void restore() {
        if (restored_) return;
        std::fflush(stdout);
        std::fflush(stderr);
        if (out_dup_ >= 0) {
            dup2(out_dup_, STDOUT_FILENO);
            close(out_dup_);
            out_dup_ = -1;
        }
        if (err_dup_ >= 0) {
            dup2(err_dup_, STDERR_FILENO);
            close(err_dup_);
            err_dup_ = -1;
        }
        if (file_ != nullptr) {
            std::fclose(file_);
            file_ = nullptr;
        }
        restored_ = true;
    }

private:
    int out_dup_ = -1;
    int err_dup_ = -1;
    FILE *file_ = nullptr;
    bool restored_ = false;
};

static void reset_festival_globals() {
    festival_cancel_requested.store(0);
    time_limit = 600;
    verbose = 1;
    level_id = 0;
    height = 0;
    width = 0;
    cores_num = 1;
    any_core_solved = 0;
    YASC_mode = 0;
    save_best_flag = 0;
    extra_mem = 0;
    just_one_level = -1;
    global_from_level = -1;
    global_to_level = -1;
    forced_alg = -1;
    start_time = 0;
    end_time = 0;
    patterns_num = 0;
    pull_patterns_num = 0;
    total_sol_time = 0;
    total_levels_tried = 0;
    total_levels_solved = 0;
    global_total_pushes = 0;
    global_total_moves = 0;
    std::strcpy(global_dir, ".");
    std::strcpy(global_level_set_name, "Android");
    global_output_filename[0] = '\0';
    global_fail_reason[0] = '\0';
}

extern "C" JNIEXPORT jstring JNICALL
Java_my_boxman_solver_FestivalSolver_solveNative(
        JNIEnv *env,
        jclass,
        jstring levelText,
        jstring filesDir,
        jint timeLimitSec,
        jint requestedCores,
        jint algorithm,
        jint extraMem,
        jboolean saveBest) {
    std::lock_guard<std::mutex> lock(solver_mutex);

    std::string dir = from_jstring(env, filesDir);
    std::string level = from_jstring(env, levelText);
    if (dir.empty()) return env->NewStringUTF("Missing files directory");
    if (level.empty()) return env->NewStringUTF("Missing level text");

    int safeTime = std::max(1, std::min(3600, static_cast<int>(timeLimitSec)));
    int safeCores = normalize_cores(static_cast<int>(requestedCores));
    int safeExtraMem = std::max(0, std::min(6, static_cast<int>(extraMem)));

    std::string inputPath = dir + "/festival-input.sok";
    std::string outputPath = dir + "/festival-output.sok";
    std::string consolePath = dir + "/festival-console.log";
    std::remove(outputPath.c_str());
    std::remove(consolePath.c_str());

    if (!write_text_file(inputPath, level)) {
        return env->NewStringUTF("Could not write input level");
    }

    reset_festival_globals();
    chdir(dir.c_str());

    std::vector<std::string> args = {
            "festival",
            inputPath,
            "-level", "1",
            "-time", std::to_string(safeTime),
            "-cores", std::to_string(safeCores),
            "-out_dir", dir,
            "-out_file", outputPath
    };
    if (algorithm >= 0 && algorithm <= 7) {
        args.push_back("-alg");
        args.push_back(std::to_string(static_cast<int>(algorithm)));
    }
    if (safeExtraMem > 0) {
        args.push_back("-extra_mem");
        args.push_back(std::to_string(safeExtraMem));
    }
    if (saveBest == JNI_TRUE) {
        args.push_back("-save_best");
    }

    std::vector<char *> argv;
    argv.reserve(args.size());
    for (std::string &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    int rc = -1;
    std::string output;
    bool nativeSolved = false;
    bool solvedMarkerInConsole = false;
    try {
        StdioRedirect redirect(consolePath);
        rc = festival_cli_main(static_cast<int>(argv.size()), argv.data());
        redirect.restore();
        std::string consoleOutput = read_text_file(consolePath);
        std::string solutionOutput = read_text_file(outputPath);
        solvedMarkerInConsole = contains_solved_marker(consoleOutput);
        nativeSolved = solvedMarkerInConsole || (saveBest != JNI_TRUE && contains_solution_path(solutionOutput));
        if (!consoleOutput.empty()) {
            output += "[festival console]\n";
            output += consoleOutput;
        }
        if (!solutionOutput.empty()) {
            if (!output.empty()) output += "\n";
            output += "[festival solution]\n";
            output += solutionOutput;
        }
        if (nativeSolved && !solvedMarkerInConsole) {
            if (!output.empty()) output += "\n";
            output += "[festival status]\nSOLVED!\n";
        }
    } catch (const std::exception &e) {
        std::string consoleOutput = read_text_file(consolePath);
        output = "Solver error: ";
        output += e.what();
        output += "\n";
        if (!consoleOutput.empty()) {
            output += "[festival console]\n";
            output += consoleOutput;
        }
    } catch (...) {
        std::string consoleOutput = read_text_file(consolePath);
        output = "Solver error: unknown native failure\n";
        if (!consoleOutput.empty()) {
            output += "[festival console]\n";
            output += consoleOutput;
        }
    }
    if (output.empty()) {
        output = "No solution file was produced.\n";
        if (global_fail_reason[0] != '\0') {
            output += "Reason: ";
            output += global_fail_reason;
            output += "\n";
        }
    }
    if (!nativeSolved) {
        std::string reason = global_fail_reason[0] == '\0' ? "" : global_fail_reason;
        if (reason.empty() || reason == "Unknown reason") {
            reason = "Search ended without a solution";
        }
        output += "\n[festival reason]\n";
        output += reason;
        output += "\n";
        output += "[festival requested time ";
        output += std::to_string(safeTime);
        output += " seconds]\n";
    }

    output += "\n[festival exit code ";
    output += std::to_string(rc);
    output += "]";
    return env->NewStringUTF(output.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_my_boxman_solver_FestivalSolver_cancelNative(JNIEnv *, jclass) {
    festival_cancel_requested.store(1);
}
