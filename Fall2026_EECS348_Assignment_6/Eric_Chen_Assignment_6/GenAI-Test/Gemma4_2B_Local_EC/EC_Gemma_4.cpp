/*
  name: EECS 348 Assignment 5
  purpose: Solve Sudoku puzzles with recursive backtracking.
  outputs: Print each puzzle and its solutions, and write results to a single solutions.txt file.
  collaborators: KIMI K2.5 | Thinking, Lauryn Burroughs, ClaudeAI
  author: Eric Chen
  creation date: 10/15/2026
  Revision Date: 10/29/2026
  Revisions: Modified output to write all solutions to solutions.txt.
*/

#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class Sudoku {
public:
    // Loads and validates the puzzle format from a file.
    bool read(const std::string& filename) {
        name = filename;
        solutions = 0;
        std::ifstream in(filename);
        if (!in) {
            std::cerr << "Could not open " << filename << "\n";
            return false;
        }
        grid = {};
        std::string tok;
        int count = 0;
        bool first = true;

        while (in >> tok) {
            if (first) { // Strip a UTF-8 BOM if the file has one.
                first = false;
                if (tok.size() >= 3 && tok.compare(0, 3, "\xEF\xBB\xBF") == 0) tok.erase(0, 3);
                if (tok.empty()) continue;
            }
            if (count >= 81) {
                std::cerr << filename << ": more than 81 cells.\n";
                return false;
            }
            int value = 0;
            if (tok == "_" || tok == "0") {
                value = 0;
            } else if (tok.size() == 1 && std::isdigit(static_cast<unsigned char>(tok[0]))) {
                value = tok[0] - '0';
            } else {
                std::cerr << filename << ": invalid token '" << tok << "'.\n";
                return false;
            }
            grid[count / 9][count % 9] = value;
            ++count;
        }

        if (count != 81) {
            std::cerr << filename << ": expected 81 cells but found " << count << ".\n";
            return false;
        }
        consistent = initialGridIsConsistent();
        return true;
    }

    // Solve this puzzle; print to console and write to 'os'.
    void run(std::ostream& os) {
        std::cout << "File: " << name << "\n";
        printGrid(std::cout);
        std::cout << "\n";

        os << "Puzzle: " << name << "\n";

        if (!consistent) {
            std::cout << "No solution found.\n";
            os << "No solution found.\n";
        } else {
            solve(os);
            if (solutions == 0) {
                std::cout << "No solution found.\n";
                os << "No solution found.\n";
            } else {
                std::cout << "Solutions found: " << solutions << "\n";
                os << "Solutions found: " << solutions << "\n";
            }
        }
        std::cout << "\n";
        os << "\n";
    }

private:
    std::string name;
    std::array<std::array<int, 9>, 9> grid{};
    bool consistent = true;
    int solutions = 0;

    bool initialGridIsConsistent() const {
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                int v = grid[r][c];
                if (v == 0) continue;

                for (int c2 = 0; c2 < 9; ++c2)
                    if (c != c2 && grid[r][c2] == v) return false;
                for (int r2 = 0; r2 < 9; ++r2)
                    if (r != r2 && grid[r2][c] == v) return false;

                int br = r / 3 * 3, bc = c / 3 * 3;
                for (int r2 = br; r2 < br + 3; ++r2)
                    for (int c2 = bc; c2 < bc + 3; ++c2)
                        if ((r2 != r || c2 != c) && grid[r2][c2] == v) return false;
            }
        }
        return true;
    }

    bool findEmpty(int& er, int& ec) const {
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                if (grid[r][c] == 0) { er = r; ec = c; return true; }
        return false;
    }

    std::vector<int> candidates(int r, int c) const {
        std::vector<int> out;
        if (grid[r][c] != 0) return out;

        bool used[10] = {};
        for (int c2 = 0; c2 < 9; ++c2) used[grid[r][c2]] = true;
        for (int r2 = 0; r2 < 9; ++r2) used[grid[r2][c]] = true;

        int br = r / 3 * 3, bc = c / 3 * 3;
        for (int r2 = br; r2 < br + 3; ++r2)
            for (int c2 = bc; c2 < bc + 3; ++c2)
                used[grid[r2][c2]] = true;

        for (int v = 1; v <= 9; ++v)
            if (!used[v]) out.push_back(v);
        return out;
    }

    void solve(std::ostream& os) {
        int r, c;
        if (!findEmpty(r, c)) {
            ++solutions;
            std::cout << "Solution " << solutions << ":\n";
            printGrid(std::cout);
            std::cout << "\n";
            printGrid(os);
            os << "\n";
            return;
        }

        for (int v : candidates(r, c)) {
            grid[r][c] = v;
            solve(os);
            grid[r][c] = 0;
        }
    }

    // Writes the grid in the puzzle file format (works for cout or a file).
    void printGrid(std::ostream& os) const {
        for (int r = 0; r < 9; ++r) {
            for (int c = 0; c < 9; ++c) {
                if (c > 0) os << ' ';
                if (grid[r][c] == 0) os << '_';
                else os << grid[r][c];
            }
            os << "\n";
        }
    }
};

int main(int argc, char** argv) {
    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i) files.emplace_back(argv[i]);

    if (files.empty()) {
        std::cout << "Enter puzzle filenames one at a time. Press Enter on a blank line when finished.\n";
        while (true) {
            std::cout << "Puzzle filename: ";
            std::string filename;
            if (!std::getline(std::cin, filename)) break;
            if (filename.empty()) break;
            files.push_back(filename);
        }
    }

    if (files.empty()) {
        std::cerr << "Error: No puzzle filenames were provided.\n";
        return 1;
    }

    std::ofstream solutions_file("solutions.txt");
    if (!solutions_file.is_open()) {
        std::cerr << "Error: Could not open 'solutions.txt' for writing.\n";
        return 1;
    }

    int failedFiles = 0;
    for (const std::string& filename : files) {
        Sudoku sudoku;
        if (sudoku.read(filename)) {
            sudoku.run(solutions_file);
        } else {
            solutions_file << "Puzzle: " << filename << "\nError: could not read puzzle.\n\n";
            ++failedFiles;
        }
    }

    solutions_file.close();

    if (failedFiles > 0) {
        std::cerr << "Finished with " << failedFiles << " file(s) that had errors.\n";
        return 1;
    }

    std::cout << "All puzzles processed. Results written to solutions.txt\n";
    return 0;
}