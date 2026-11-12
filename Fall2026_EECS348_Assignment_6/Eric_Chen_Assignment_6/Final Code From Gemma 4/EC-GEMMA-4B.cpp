/*
  name: EECS 348 Assignment 5
  purpose: Solve Sudoku puzzles with recursive backtracking.
  outputs: Print each puzzle and its solutions, and write results to a single solutions.txt file.
  collaborators: KIMI K2.5 | Thinking, Lauryn Burroughs, ClaudeAI | Gemma 4 2B Thinking. Eric Chen
  author: Eric Chen
  creation date: 10/15/2026
  Revision Date: 10/29/2026
  Revision Date: 11/12/2026
  Revisions(1): Modified output to write all solutions to solutions.txt.
  Revisions: Added bitmask constraint tracking, minimum-remaining-values cell choice,
             baseName() for portable labels, single printGrid(ostream), and split file I/O out of main.
*/

#include <array> // fixed-size array type used for the 9x9 Sudoku grid
#include <fstream> // std::ifstream, std::ofstream for file handling
#include <iostream> // Provides standard output and error streams.
#include <string> // Provides std::string for filenames and input tokens.
#include <vector> // Provides std::vector for filenames.

// Returns the filename without its directory part so labels match on Linux and Windows.
std::string baseName(const std::string& path) {
    std::size_t slash = path.find_last_of("/\\"); // Find the last separator of either style.
    if (slash == std::string::npos) return path; // No directory part, so use the name as given.
    return path.substr(slash + 1); // Keep only the text after the last separator.
}

// Counts the set bits in a candidate mask.
int countBits(int mask) {
    int n = 0; // Running total of set bits.
    while (mask) { // Loop until no bits remain.
        mask &= mask - 1; // Clear the lowest set bit.
        ++n; // Count it.
    }
    return n; // Number of candidate digits.
}

class Sudoku { // Stores a puzzle and provides the functions needed to solve it.
public:
    // --- Public Interface ---

    // Loads and validates the puzzle format from a file.
    bool read(const std::string& filename) {
        name = filename; // Save the filename for later output and numbering.
        solutions = 0; // Reset the solution count for this puzzle.
        std::ifstream in(filename); // Open the file for reading.
        if (!in) in.open(filename + ".txt"); // If that fails, also try with a .txt extension.
        if (!in) { // Check whether the file opened successfully.
            std::cerr << "Could not open " << filename << " (or " << filename
                      << ".txt). Check the path and working directory.\n"; // Report the file-opening error.
            return false; // Tell main that the puzzle could not be read.
        }
        grid.fill({}); // Clear every row of the grid before loading new values.
        int count = 0; // Track how many of the 81 cells have been read.
        char ch; // Hold the next character from the file.

        while (in.get(ch)) { // Read the file one character at a time.
            int value = 0; // Use zero to represent an empty cell.
            if (ch == '_' || ch == '.' || ch == '0') { // Interpret an underscore, period, or zero as an empty cell.
                value = 0; // Keep empty cells represented by zero in the grid.
            } else if (ch >= '1' && ch <= '9') { // Accept a digit from 1 to 9 as a clue.
                value = ch - '0'; // Convert the digit character to its integer value.
            } else {
                continue; // Skip spaces, newlines, and any other separator characters.
            }

            if (count >= 81) { // Reject input containing more than 81 cells.
                std::cerr << filename << ": more than 81 cells.\n"; // Report the problem.
                return false; // Tell main that the puzzle could not be read.
            }
            grid[count / 9][count % 9] = value; // Map the cell count to its row and column, then store the value.
            ++count; // Advance to the next grid cell.
        }

        if (count != 81) { // Reject input that does not contain exactly 81 cells.
            std::cerr << filename << ": expected 81 cells but found " << count << ".\n"; // Report the problem.
            return false; // Tell main that the puzzle could not be read.
        }
        consistent = initialGridIsConsistent(); // Remember whether the given values follow Sudoku rules.
        return true; // The file has a valid format, so main can run the solver.
    }

    // Solve this puzzle and report results to the provided output stream 'os'.
    void run(std::ostream& os) {
        std::cout << "File: " << baseName(name) << "\n"; // Display the name of the puzzle file.
        printGrid(std::cout); // Show the puzzle before attempting to solve it.
        std::cout << "\n"; // Leave a blank line after the starting grid.

        // Write the required header for solutions.txt
        os << "Puzzle: " << baseName(name) << "\n";

        if (!consistent) { // Skip solving if the starting clues already violate Sudoku rules.
            report("No solution found.", os); // Write the same result when the output stream is available.
        } else {
            solve(os); // Search recursively for every valid solution, passing 'os'.

            if (solutions == 0) { // Check whether the search found any complete grid.
                report("No solution found.", os); // Report that no solution exists.
            } else {
                report("Solutions found: " + std::to_string(solutions), os); // Save the solution count.
            }
        }
        std::cout << "\n"; // Add a blank line after this puzzle's results on the console.
        os << "\n"; // Add a blank line after this puzzle's results.
    }


private: // Keep the puzzle data and solver helpers inside the class.
    std::string name; // Store the input filename.
    std::array<std::array<int, 9>, 9> grid{}; // Store the 9-by-9 grid, with zero for empty cells.
    std::array<int, 9> rowMask{}; // Bit v is set when digit v is already used in that row.
    std::array<int, 9> colMask{}; // Bit v is set when digit v is already used in that column.
    std::array<int, 9> boxMask{}; // Bit v is set when digit v is already used in that 3x3 block.
    bool consistent = true; // Record whether the starting clues follow Sudoku rules.
    int solutions = 0; // Count the completed solutions found by the search.

    static const int ALL_DIGITS = 0x3FE; // Bits 1 through 9 set, one bit for each Sudoku digit.

    // --- Helper Functions ---

    // Returns the index (0-8) of the 3x3 block containing row r and column c.
    static int boxOf(int r, int c) { return (r / 3) * 3 + (c / 3); }

    // Writes one line to both the console and the output stream.
    static void report(const std::string& msg, std::ostream& os) {
        std::cout << msg << "\n"; // Console copy of the message.
        os << msg << "\n"; // File copy of the message.
    }

    // Checks whether the starting clues contain any Sudoku rule violations, and builds the bitmasks.
    bool initialGridIsConsistent() {
        rowMask.fill(0); // Clear the row masks before loading clues.
        colMask.fill(0); // Clear the column masks before loading clues.
        boxMask.fill(0); // Clear the block masks before loading clues.
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                int v = grid[r][c];
                if (v == 0) continue;

                int bit = 1 << v; // Mask bit for this digit.
                int b = boxOf(r, c); // Block containing this cell.
                // A digit already marked in the row, column, or block is a duplicate clue.
                if ((rowMask[r] | colMask[c] | boxMask[b]) & bit) return false;
                rowMask[r] |= bit; // Record the digit as used in its row.
                colMask[c] |= bit; // Record the digit as used in its column.
                boxMask[b] |= bit; // Record the digit as used in its block.
            }
        }
        return true; // No duplicate clues were found in any row, column, or block.
    }

    // Places digit v at row r and column c and updates the masks.
    void place(int r, int c, int v) {
        int bit = 1 << v; // Mask bit for this digit.
        grid[r][c] = v; // Place this candidate in the empty cell.
        rowMask[r] |= bit; // Mark the digit as used in the row.
        colMask[c] |= bit; // Mark the digit as used in the column.
        boxMask[boxOf(r, c)] |= bit; // Mark the digit as used in the block.
    }

    // Removes digit v from row r and column c and updates the masks.
    void unplace(int r, int c, int v) {
        int bit = 1 << v; // Mask bit for this digit.
        grid[r][c] = 0; // Remove the trial value so the next candidate can be tested.
        rowMask[r] &= ~bit; // Free the digit in the row.
        colMask[c] &= ~bit; // Free the digit in the column.
        boxMask[boxOf(r, c)] &= ~bit; // Free the digit in the block.
    }

    // Recursive backtracking solver (now accepts std::ostream& os).
    void solve(std::ostream& os) {
        int bestR = -1, bestC = -1; // Location of the most constrained empty cell.
        int bestMask = 0; // Legal digits for that cell.
        int bestCount = 10; // Fewest legal digits seen so far (10 means no empty cell seen yet).

        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (grid[r][c] != 0) continue; // Skip cells that are already filled.
                int avail = ALL_DIGITS & ~(rowMask[r] | colMask[c] | boxMask[boxOf(r, c)]); // Digits not yet used by row, column, or block.
                int n = countBits(avail); // Number of legal digits for this cell.
                if (n == 0) return; // An empty cell with no legal digit means this path is a dead end.
                if (n < bestCount) { // Prefer the cell with the fewest legal digits.
                    bestCount = n; // Remember the new minimum.
                    bestR = r; // Remember its row.
                    bestC = c; // Remember its column.
                    bestMask = avail; // Remember its legal digits.
                }
            }
        }

        if (bestR == -1) { // A full grid means the current arrangement is a solution.
            ++solutions; // Count this completed solution.
            std::cout << "Solution " << solutions << ":\n"; // Label the solution on the console.
            printGrid(std::cout); // Show the completed grid on the console.
            std::cout << "\n"; // Separate this solution from the next one on the console.

            // Write the completed grid to the output stream.
            printGrid(os);
            os << "\n"; // Separate this solution from the next one in the file.
            return; // Stop this search path because the grid is complete.
        }

        for (int v = 1; v <= 9; ++v) { // Try each legal digit in turn.
            if (!(bestMask & (1 << v))) continue; // Skip digits that are not legal in this cell.
            place(bestR, bestC, v); // Place this candidate in the empty cell.
            solve(os); // Continue searching with this choice in place, passing 'os'.
            unplace(bestR, bestC, v); // Remove the trial value so the next candidate can be tested.
        }
    }

    // Writes the current grid to the provided output stream 'os' in the puzzle file format.
    void printGrid(std::ostream& os) const {
        for (int r = 0; r < 9; ++r) { // Write one row at a time.
            for (int c = 0; c < 9; ++c) { // Write one cell at a time.
                if (c > 0) os << ' '; // Separate cells with spaces, but not before the first cell.
                if (grid[r][c] == 0) os << '_'; // Write an underscore for an empty cell.
                else os << grid[r][c]; // Write the digit for a filled cell.
            }
            os << "\n"; // End the current row in the output.
        }
    }
};

// Read puzzle files from command-line arguments, or ask for them interactively.
std::vector<std::string> collectFilenames(int argc, char** argv) {
    std::vector<std::string> files; // Store puzzle filenames from arguments or interactive input.

    for (int i = 1; i < argc; ++i) {
        files.emplace_back(argv[i]);
    }

    if (files.empty()) { // Ask for filenames if none were provided as arguments.
        std::cout << "Enter puzzle filenames one at a time. Press Enter on a blank line when finished.\n";
        while (true) {
            std::cout << "Puzzle filename: ";
            std::string filename;
            if (!std::getline(std::cin, filename)) break;
            if (filename.empty()) break;
            files.push_back(filename);
        }
    }
    return files; // Hand the collected filenames back to main.
}

// Solves every puzzle and writes the results; returns the number of failed files, or -1 if the output file fails.
int processPuzzles(const std::vector<std::string>& files, const std::string& outPath) {
    // --- Consolidated Output Setup ---
    // Open the single output file for all solutions
    std::ofstream solutions_file(outPath);
    if (!solutions_file.is_open()) {
        std::cerr << "Error: Could not open '" << outPath << "' for writing.\n";
        return -1; // Signal an output-file error to main.
    }
    // ----------------------------------

    int failedFiles = 0;

    // Loop through all files and process them
    for (const std::string& filename : files) {
        Sudoku sudoku;

        if (sudoku.read(filename)) {
            // Pass the single output stream to the run method
            sudoku.run(solutions_file);
        } else {
            solutions_file << "Puzzle: " << baseName(filename) << "\nError: could not read puzzle.\n\n"; // Record the failure in the file.
            failedFiles++;
        }
    }

    // Close the file
    solutions_file.close();
    return failedFiles; // Report how many puzzles could not be read.
}

// Main function to manage the puzzle processing
int main(int argc, char** argv) {
    std::vector<std::string> files = collectFilenames(argc, argv); // Gather the puzzle filenames.

    if (files.empty()) { // Check whether any filenames were collected.
        std::cerr << "Error: No puzzle filenames were provided.\n";
        return 1;
    }

    int failedFiles = processPuzzles(files, "solutions.txt"); // Solve everything and write solutions.txt.
    if (failedFiles < 0) return 1; // The output file could not be opened.

    if (failedFiles > 0) {
        std::cerr << "Finished with " << failedFiles
                  << " file(s) that had errors. Check File for Strings or misformatting.\n";
        return 1;
    }

    std::cout << "All puzzles processed. Results written to solutions.txt\n";
    return 0;
}