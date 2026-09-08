#include <vector>
#include <array>

class Simulation {
public:
    void iterate() {
        std::vector<std::array<int, 2>> cells_to_kill;
        std::vector<std::array<int, 2>> cells_to_revive;
        for (int row = 0; row < 16; row++) {
            for (int column = 0; column < 16; column++) {
                bool (current_cell_state) = _sim_buffer_ptr[row][column];
                int alive_neighbours = get_alive_neighbour_count(row, column);
                if (alive_neighbours < 2 || alive_neighbours > 3) {
                    cells_to_kill.push_back(std::array<int, 2> {row, column});
                    continue;
                }
                if (alive_neighbours == 3 && (!current_cell_state)) {
                    cells_to_revive.push_back(std::array<int, 2> {row, column});
                }
            }
        }
        kill_cells(cells_to_kill);
        revive_cells(cells_to_revive);
    }

    Simulation(bool (*&sim_buffer_ptr)[16]) {
        _sim_buffer_ptr = sim_buffer_ptr;
    }

private:

    bool (*_sim_buffer_ptr)[16];

    int get_alive_neighbour_count(int row, int column) {
        int alive_neighbours = 0;
        for (int neighbour_row = row - 1; neighbour_row <= row + 1; neighbour_row++) {
            if (neighbour_row == -1 || neighbour_row == 16) continue;
            for (int neighbour_column = column - 1; neighbour_column <= column+1; neighbour_column++) {
                if ((neighbour_row == row && neighbour_column == column) || neighbour_column == -1 || neighbour_column == 16) continue;
                bool neighbour_cell_state = _sim_buffer_ptr[neighbour_row][neighbour_column];
                if (neighbour_cell_state) {
                    alive_neighbours++;
                }
            }
        }
        return alive_neighbours;
    }

    void kill_cells(std::vector<std::array<int, 2>> cells_to_kill) {
        for (int i = 0; i < cells_to_kill.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_kill[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            _sim_buffer_ptr[row][column] = 0;
        };
    }

    void revive_cells(std::vector<std::array<int, 2>> cells_to_revive) {
        for (int i = 0; i < cells_to_revive.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_revive[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            _sim_buffer_ptr[row][column] = 1;
        };
    }
};