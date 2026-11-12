/*
 name: EECS 348 Assignment 5
 purpose: Solve Sudoku puzzles with recursive backtracking.
 outputs: Print each puzzle and its solutions, and write all results to a single solutions.txt file.
 collaborators: KIMI K2.5 | Thinking, Lauryn Burroughs, ClaudeAI
 author: Eric Chen
 creation date: 10/15/2026
 Revision Date: 11/12/2026
 Revisions: Replaced the per-puzzle solutionN.txt files with one solutions.txt file.
 Each puzzle section is: "Puzzle: <filename>", then either "No solution found."
 or every solution (puzzle file format) followed by "Solutions found: n".
*/

#include <array>    // fixed-size array type used for the 9x9 Sudoku grid
#include <cctype>   // checks whether a character is a decimal digit
#include <fstream>  // std::ifstream and std::ofstream for file input and output
#include <iostream> // Provides standard output and error streams.
#include <string>   // Provides std::string for filenames and input tokens.
#include <vector>   // Provides std::vector for candidates and filenames.

class Sudoku { // Stores a puzzle and provides the functions needed to solve it.
public:
    // Original code block from the author.
    bool read(const std::string& filename) { // Loads and validates the puzzle format from a file.
        name = filename; // Save the filename for later output.
        std::ifstream in(filename); // Open the file for reading.
        if (!in) { // Check whether the file opened successfully.
            std::cerr << "Could not open " << filename << "\n"; // Report the file-opening error.
            return false; // Tell main that the puzzle could not be read.
        }
        grid.fill({}); // Clear every row of the grid before loading new values.
        std::string tok; // Hold the next whitespace-separated input token.
        int count = 0; // Track how many of the 81 cells have been read.

        while (in >> tok) { // Read each whitespace-separated cell token.
            if (count >= 81) return false; // Reject input containing more than 81 cells.
            int value = 0; // Use zero to represent an empty cell.
            if (tok == "_") { // Interpret an underscore as an empty cell.
                value = 0; // Keep empty cells represented by zero in the grid.
            } else if (tok.size() == 1 && std::isdigit(static_cast<unsigned char>(tok[0]))) { // Accept a single digit token.
                value = tok[0] - '0'; // Convert the digit character to its integer value.
                if (value < 1 || value > 9) return false; // Reject zero and values outside Sudoku's 1-9 range.
            } else {
                return false; // Reject tokens that are not underscores or single digits.
            }

            grid[count / 9][count % 9] = value; // Map the cell count to its row and column, then store the value.
            ++count; // Advance to the next grid cell.
        }

        if (count != 81) return false; // Reject input that does not contain exactly 81 cells.
        consistent = initialGridIsConsistent(); // Remember whether the given values follow Sudoku rules.
        return true; // The file has a valid format, so main can run the solver.
    }

    // Modified: writes into the shared solutions.txt stream instead of a per-puzzle file.
    void run(std::ostream& out) { // Solve this puzzle and report results to the console and the shared output stream.
        outFile = &out; // Remember the shared output stream so solve() can write each solution.
        std::cout << "File: " << name << "\n"; // Display the name of the puzzle file.
        printGrid(); // Show the puzzle before attempting to solve it.

        out << "Puzzle: " << baseName() << "\n"; // Required header line: the puzzle file name.

        if (!consistent) { // Skip solving if the starting clues already violate Sudoku rules.
            std::cout << "No solution found\n\n"; // Report that this puzzle has no valid solution.
            out << "No solution found.\n\n"; // Write the same result to solutions.txt.
            return; // Finish processing this inconsistent puzzle.
        }

        solve(); // Search recursively for every valid solution.

        if (solutions == 0) { // Check whether the search found any complete grid.
            std::cout << "No solution found\n"; // Report that no solution exists.
            out << "No solution found.\n"; // Write the result to solutions.txt.
        } else {
            std::cout << "Solutions found: " << solutions << "\n"; // Display the number of solutions found.
            out << "Solutions found: " << solutions << "\n"; // Save the solution count to solutions.txt.
        }
        std::cout << "\n"; // Add a blank line after this puzzle's results.
        out << "\n"; // Separate this puzzle's section from the next one in the file.
    }

    // New code: records a puzzle that could not be read so solutions.txt still lists it.
    void writeUnreadable(std::ostream& out) const { // Write the header and a no-solution line for a bad file.
        out << "Puzzle: " << baseName() << "\n"; // Header line with the file name.
        out << "No solution found.\n\n"; // A puzzle that cannot be read has no solutions.
    }

    // New code: lets main set the name before a failed read is reported.
    void setName(const std::string& filename) { name = filename; } // Store the filename.

private: // Keep the puzzle data and solver helpers inside the class.
    std::string name; // Store the input filename.
    std::array<std::array<int, 9>, 9> grid{}; // Store the 9-by-9 grid, with zero for empty cells.
    bool consistent = true; // Record whether the starting clues follow Sudoku rules.
    int solutions = 0; // Count the completed solutions found by the search.
    std::ostream* outFile = nullptr; // Points to the shared solutions.txt stream.

    // New code: returns the filename without any folder path (e.g. "puzzle1.txt").
    std::string baseName() const { // Strip the directory portion of the stored path.
        std::size_t pos = name.find_last_of("/\\"); // Find the last folder separator.
        return (pos == std::string::npos) ? name : name.substr(pos + 1); // Keep only the file name.
    }

    // Original code block from the author.
    bool initialGridIsConsistent() { // Check whether the starting clues contain any Sudoku rule violations.
        for (int r = 0; r < 9; ++r) { // Visit each row in the grid.
            for (int c = 0; c < 9; ++c) { // Visit each cell in the current row.
                int v = grid[r][c]; // Store the current cell's value for the checks below.
                if (v == 0) continue; // Empty cells cannot conflict with a given digit.

                for (int c2 = c + 1; c2 < 9; ++c2) { // Check later cells in this row for the same digit.
                    if (grid[r][c2] == v) return false; // A repeated digit makes the starting grid invalid.
                }
                for (int r2 = r + 1; r2 < 9; ++r2) { // Check later cells in this column for the same digit.
                    if (grid[r2][c] == v) return false; // A repeated digit makes the starting grid invalid.
                }

                int br = r / 3 * 3; // Find the first row of this cell's 3-by-3 block.
                int bc = c / 3 * 3; // Find the first column of this cell's 3-by-3 block.
                for (int r2 = br; r2 < br + 3; ++r2) { // Visit each row in the block.
                    for (int c2 = bc; c2 < bc + 3; ++c2) { // Visit each column in the block.
                        if ((r2 != r || c2 != c) && grid[r2][c2] == v) return false; // Reject a matching digit elsewhere in the block.
                    }
                }
            }
        }
        return true; // No duplicate clues were found in any row, column, or block.
    }

    // Original code block from the author.
    bool findEmpty(int& er, int& ec) const { // Find an empty cell and return its location through er and ec.
        for (int r = 0; r < 9; ++r) { // Search the grid one row at a time.
            for (int c = 0; c < 9; ++c) { // Check each column in the current row.
                if (grid[r][c] == 0) { // Check whether this cell is empty.
                    er = r; // Save the empty cell's row index.
                    ec = c; // Save the empty cell's column index.
                    return true; // Report that an empty cell was found.
                }
            }
        }
        return false; // Report that every cell is filled.
    }

    // Original code block from the author.
    std::vector<int> candidates(int r, int c) const { // Return digits that can legally go in the cell at row r and column c.
        std::vector<int> out; // Collect the digits that are allowed in this cell.
        if (grid[r][c] != 0) return out; // Return no candidates if the cell is already filled.

        bool used[10] = {}; // Track which digits are already present in the cell's row, column, or block.

        for (int c2 = 0; c2 < 9; ++c2) used[grid[r][c2]] = true; // Mark digits already used in this row.
        for (int r2 = 0; r2 < 9; ++r2) used[grid[r2][c]] = true; // Mark digits already used in this column.

        int br = r / 3 * 3; // Find the first row of the cell's 3-by-3 block.
        int bc = c / 3 * 3; // Find the first column of the cell's 3-by-3 block.
        for (int r2 = br; r2 < br + 3; ++r2) { // Visit each row of the block.
            for (int c2 = bc; c2 < bc + 3; ++c2) { // Visit each column of the block.
                used[grid[r2][c2]] = true; // Mark digits already used in the block.
            }
        }

        for (int v = 1; v <= 9; ++v) { // Check each Sudoku digit for availability.
            if (!used[v]) out.push_back(v); // Add the digit if it is unused in the row, column, and block.
        }
        return out; // Return all legal choices for this cell.
    }

    // Original code block from the author (output now goes to the shared stream).
    void solve() { // Use recursive backtracking to find and report every valid solution.
        int r, c; // Hold the row and column of the next empty cell.
        if (!findEmpty(r, c)) { // A full grid means the current arrangement is a solution.
            ++solutions; // Count this completed solution.
            std::cout << "Solution " << solutions << ":\n"; // Label this solution in the console output.
            printGrid(); // Display the completed grid.
            std::cout << "\n"; // Separate this solution from the next one.
            if (outFile) { // Write the grid only when the output stream is available.
                writeGrid(*outFile); // Save the completed grid in the puzzle file format.
                *outFile << "\n"; // Separate this solution from the next one in the file.
            }
            return; // Stop this search path because the grid is complete.
        }

        std::vector<int> vals = candidates(r, c); // Get the legal digits for the selected empty cell.
        for (int v : vals) { // Try each legal digit in turn.
            grid[r][c] = v; // Place this candidate in the empty cell.
            solve(); // Continue searching with this choice in place.
            grid[r][c] = 0; // Remove the trial value so the next candidate can be tested.
        }
    }

    // Original code block from the author.
    void printGrid() const { // Print the current grid to standard output.
        for (int r = 0; r < 9; ++r) { // Print each row in order.
            for (int c = 0; c < 9; ++c) { // Print each cell in the current row.
                if (grid[r][c] == 0) std::cout << "_ "; // Display empty cells as underscores.
                else std::cout << grid[r][c] << ' '; // Display a filled cell's digit.
            }
            std::cout << "\n"; // Move to the next output line after this row.
        }
    }

    // New code: writes the grid to a stream in the same format as the puzzle files.
    void writeGrid(std::ostream& os) const { // Write the current grid to the provided output stream.
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

// Modified: opens one solutions.txt and shares it with every puzzle.
int main(int argc, char** argv) { // Start the program and process the supplied puzzle files.
    std::vector<std::string> files; // Store puzzle filenames from arguments or interactive input.

    for (int i = 1; i < argc; ++i) { // Read each command-line argument after the program name.
        files.emplace_back(argv[i]); // Add this puzzle filename to the list.
    }

    if (files.empty()) { // Ask for filenames if none were provided as arguments.
        std::cout << "Enter puzzle filenames one at a time. Press Enter on a blank line when finished.\n"; // Explain how to enter filenames.
        while (true) { // Keep reading filenames until input ends or a blank line is entered.
            std::cout << "Puzzle filename: "; // Prompt for the next puzzle filename.
            std::string filename; // Hold the filename entered by the user.
            if (!std::getline(std::cin, filename)) break; // Stop if input is unavailable or reaches end-of-file.
            if (filename.empty()) break; // Stop when the user enters a blank line.
            files.push_back(filename); // Add the entered filename to the list.
        }
    }

    if (files.empty()) { // Check whether any filenames were collected.
        std::cerr << "Error: No puzzle filenames were provided.\n"; // Report that there are no puzzles to process.
        return 1; // Exit with an error status because no files were provided.
    }

    std::ofstream solutionsFile("solutions.txt"); // Create or overwrite the single results file.
    if (!solutionsFile) { // Check whether the results file opened successfully.
        std::cerr << "Could not write solutions.txt\n"; // Report that the output file could not be created.
        return 1; // Exit with an error status because results cannot be saved.
    }

    int failedFiles = 0; // Count files that could not be read or had invalid input.
    for (const std::string& filename : files) { // Process each puzzle filename in order.
        Sudoku sudoku; // Create a solver instance for this puzzle.
        sudoku.setName(filename); // Store the name so a failed read can still be recorded.
        if (sudoku.read(filename)) { // Load the puzzle and check whether its file format is valid.
            sudoku.run(solutionsFile); // Solve it and append its results to solutions.txt.
        } else {
            sudoku.writeUnreadable(solutionsFile); // Record the unreadable puzzle in solutions.txt.
            ++failedFiles; // Record this file as failed and continue with the remaining files.
        }
    }

    if (failedFiles > 0) { // Check whether any input files failed to process.
        std::cerr << "Finished with " << failedFiles // Report how many files had errors.
                  << " file(s) that had errors. Check File for Strings or misformatting.\n"; // Suggest checking file contents and formatting.
        return 1; // Exit with an error status because at least one file failed.
    }

    return 0; // Exit successfully because every file was processed.
}
