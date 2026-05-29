package my.boxman.solver;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

public final class FestivalSolver {
    public static final int DEFAULT_TIME_LIMIT_SEC = 15;
    public static final int AUTO_ALGORITHM = -1;
    public static final int[] DEFAULT_ALGORITHMS = {7, AUTO_ALGORITHM, 0, 1, 2, 3, 4, 5, 6};

    private static final Pattern LURD_LINE = Pattern.compile("[lurdLURD]+");
    private static final Pattern SOLVER_TIME = Pattern.compile("Solver time:\\s*(\\d+):(\\d+):(\\d+)");

    static {
        System.loadLibrary("festivalcore");
    }

    private FestivalSolver() {
    }

    public static synchronized String solvePath(String levelText, File workDir, int timeLimitSec) {
        String output = solveText(levelText, workDir, timeLimitSec, 7);
        if (!wasSolved(output)) {
            throw new IllegalStateException(trimMessage(output));
        }
        String path = extractPath(output);
        if (path.length() == 0) {
            throw new IllegalStateException(trimMessage(output));
        }
        return path;
    }

    public static synchronized char firstMove(String levelText, File workDir, int timeLimitSec) {
        String path = solvePath(levelText, workDir, timeLimitSec);
        return path.charAt(0);
    }

    public static synchronized String solveText(String levelText, File workDir, int timeLimitSec) {
        return solveText(levelText, workDir, timeLimitSec, 7);
    }

    public static synchronized String solveText(String levelText, File workDir, int timeLimitSec, int algorithm) {
        return solveText(levelText, workDir, timeLimitSec, 1, algorithm, 0, false);
    }

    public static synchronized String solveText(String levelText, File workDir, int timeLimitSec, int cores, int algorithm, int extraMem, boolean saveBest) {
        if (workDir == null) {
            throw new IllegalArgumentException("Missing solver work directory");
        }
        if (!workDir.exists() && !workDir.mkdirs()) {
            throw new IllegalStateException("Could not create solver work directory");
        }
        return solveNative(levelText, workDir.getAbsolutePath(), timeLimitSec, cores, algorithm, extraMem, saveBest);
    }

    public static String extractPath(String solverOutput) {
        List<String> paths = extractPaths(solverOutput);
        return paths.isEmpty() ? "" : paths.get(0);
    }

    public static List<String> extractPaths(String solverOutput) {
        ArrayList<String> paths = new ArrayList<String>();
        if (solverOutput == null || solverOutput.length() == 0) {
            return paths;
        }

        String[] lines = solverOutput.split("\\r?\\n");
        boolean afterSolutionMarker = false;
        String fallback = "";
        for (String line : lines) {
            String s = line.trim();
            if (s.equals("Solution")) {
                afterSolutionMarker = true;
                continue;
            }
            if (LURD_LINE.matcher(s).matches()) {
                if (afterSolutionMarker) {
                    addUniquePath(paths, s);
                    afterSolutionMarker = false;
                    continue;
                }
                if (s.length() > fallback.length()) {
                    fallback = s;
                }
            } else if (afterSolutionMarker && s.length() > 0) {
                afterSolutionMarker = false;
            }
        }
        if (paths.isEmpty() && fallback.length() > 0) {
            paths.add(fallback);
        }
        return paths;
    }

    public static boolean wasSolved(String solverOutput) {
        if (solverOutput == null || solverOutput.length() == 0) {
            return false;
        }
        String[] lines = solverOutput.split("\\r?\\n");
        for (String line : lines) {
            if ("SOLVED!".equals(line.trim())) {
                return true;
            }
        }
        return false;
    }

    public static int extractSolverTimeSec(String solverOutput) {
        if (solverOutput == null || solverOutput.length() == 0) {
            return -1;
        }
        java.util.regex.Matcher matcher = SOLVER_TIME.matcher(solverOutput);
        int last = -1;
        while (matcher.find()) {
            int hours = parseInt(matcher.group(1));
            int minutes = parseInt(matcher.group(2));
            int seconds = parseInt(matcher.group(3));
            last = hours * 3600 + minutes * 60 + seconds;
        }
        return last;
    }

    public static String extractReason(String solverOutput) {
        if (solverOutput == null || solverOutput.length() == 0) {
            return "";
        }
        String[] lines = solverOutput.split("\\r?\\n");
        for (int i = 0; i < lines.length; i++) {
            if ("[festival reason]".equals(lines[i].trim())) {
                for (int j = i + 1; j < lines.length; j++) {
                    String reason = lines[j].trim();
                    if (reason.length() > 0) {
                        return reason;
                    }
                }
            }
        }
        return "";
    }

    private static int parseInt(String value) {
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException ex) {
            return 0;
        }
    }

    private static void addUniquePath(ArrayList<String> paths, String path) {
        for (String item : paths) {
            if (item.equals(path)) {
                return;
            }
        }
        paths.add(path);
    }

    private static String trimMessage(String output) {
        if (output == null || output.length() == 0) {
            return "No solution was produced";
        }
        String s = output.trim();
        if (s.length() > 240) {
            return s.substring(0, 240);
        }
        return s;
    }

    public static String trimSolverMessage(String output) {
        return trimMessage(output);
    }

    private static native String solveNative(String levelText, String filesDir, int timeLimitSec, int cores, int algorithm, int extraMem, boolean saveBest);
}
