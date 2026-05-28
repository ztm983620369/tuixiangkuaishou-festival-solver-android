package my.boxman.solver;

import java.io.File;
import java.util.regex.Pattern;

public final class FestivalSolver {
    public static final int DEFAULT_TIME_LIMIT_SEC = 8;
    public static final int AUTO_ALGORITHM = -1;
    public static final int[] DEFAULT_ALGORITHMS = {7, AUTO_ALGORITHM, 0, 1, 2, 3, 4, 5, 6};

    private static final Pattern LURD_LINE = Pattern.compile("[lurdLURD]+");

    static {
        System.loadLibrary("festivalcore");
    }

    private FestivalSolver() {
    }

    public static synchronized String solvePath(String levelText, File workDir, int timeLimitSec) {
        String output = solveText(levelText, workDir, timeLimitSec, 7);
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
        if (solverOutput == null || solverOutput.length() == 0) {
            return "";
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
                    return s;
                }
                if (s.length() > fallback.length()) {
                    fallback = s;
                }
            } else if (afterSolutionMarker && s.length() > 0) {
                afterSolutionMarker = false;
            }
        }
        return fallback;
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
