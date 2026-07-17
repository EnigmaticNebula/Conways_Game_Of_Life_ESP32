#include <cell.hpp>
#include <vector>
#include <array>

class Simulation {
public:
    Cell grid[16][16];

    Simulation() {
        init_grid();
    };

    void iterate() {
        std::vector<std::array<int, 2>> cells_to_kill;
        std::vector<std::array<int, 2>> cells_to_revive;
        for (int row = 0; row < 16; row++) {
            for (int column = 0; column < 16; column++) {
                Cell current_cell = grid[row][column];
                int alive_neighbours = get_alive_neighbour_count(row, column);
                if (alive_neighbours < 2 || alive_neighbours > 3) {
                    cells_to_kill.push_back(std::array<int, 2> {row, column});
                    continue;
                }
                if (alive_neighbours = 3 && !current_cell.alive) {
                    cells_to_revive.push_back(std::array<int, 2> {row, column});
                }
            }
        }
        kill_cells(cells_to_kill);
        revive_cells(cells_to_revive);
    }
    
    bool** get_cell_states() {
        bool** cell_states = new bool*[16];
        for (int i = 0; i < 16; i++) {
            cell_states[i] = new bool[16];
        }
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                Cell current_cell = grid[i][j];
                cell_states[i][j] = current_cell.alive;
            }
        }
        return cell_states;
    }

private:

    void init_grid() {
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j = j + 1) {
                grid[i][j] = Cell(i, j);
            }
        }
    };

    int get_alive_neighbour_count(int row, int column) {
        int alive_neighbours = 0;
        Cell current_cell = grid[row][column];
        for (int neighbour_row = row - 1; neighbour_row <= row + 1; neighbour_row++) {
            for (int neighbour_column = column - 1; neighbour_column <= column-1; neighbour_column++) {
                Cell neighbour_cell = grid[neighbour_row][neighbour_column];
                if (neighbour_cell.alive) {
                    alive_neighbours++;
                }
            }
        }
        return alive_neighbours;
    }

    void kill_cells(std::vector<std::array<int, 2>> cells_to_kill) {
        for (auto i = 0; i < cells_to_kill.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_kill[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            Cell cell = grid[row][column];
            cell.kill_cell();
        };
    }

    void revive_cells(std::vector<std::array<int, 2>> cells_to_revive) {
        for (auto i = 0; i < cells_to_revive.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_revive[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            Cell cell = grid[row][column];
            cell.revive_cell();
        };
    }
};