#include <jni.h>

#include <algorithm>
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

static std::mutex solver_mutex;

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

static void reset_festival_globals() {
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
        jint requestedCores) {
    std::lock_guard<std::mutex> lock(solver_mutex);

    std::string dir = from_jstring(env, filesDir);
    std::string level = from_jstring(env, levelText);
    if (dir.empty()) return env->NewStringUTF("Missing files directory");
    if (level.empty()) return env->NewStringUTF("Missing level text");

    int safeTime = std::max(1, std::min(300, static_cast<int>(timeLimitSec)));
    (void)requestedCores;
    int safeCores = 1;

    std::string inputPath = dir + "/festival-input.sok";
    std::string outputPath = dir + "/festival-output.sok";
    std::remove(outputPath.c_str());

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
            "-alg", "7",
            "-out_dir", dir,
            "-out_file", outputPath
    };

    std::vector<char *> argv;
    argv.reserve(args.size());
    for (std::string &arg : args) {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }

    int rc = -1;
    std::string output;
    try {
        rc = festival_cli_main(static_cast<int>(argv.size()), argv.data());
        output = read_text_file(outputPath);
    } catch (const std::exception &e) {
        output = "Solver error: ";
        output += e.what();
        output += "\n";
    } catch (...) {
        output = "Solver error: unknown native failure\n";
    }
    if (output.empty()) {
        output = "No solution file was produced.\n";
        if (global_fail_reason[0] != '\0') {
            output += "Reason: ";
            output += global_fail_reason;
            output += "\n";
        }
    }

    output += "\n[festival exit code ";
    output += std::to_string(rc);
    output += "]";
    return env->NewStringUTF(output.c_str());
}
