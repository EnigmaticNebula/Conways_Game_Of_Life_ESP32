#include <vector>
#include <array>

class Simulation {
public:
    void iterate(bool (*prev_frame)[16], bool (*new_frame)[16]) {
        std::vector<std::array<int, 2>> cells_to_kill;
        std::vector<std::array<int, 2>> cells_to_revive;
        for (int row = 0; row < 16; row++) {
            for (int column = 0; column < 16; column++) {
                bool current_cell_state = prev_frame[row][column];
                int alive_neighbours = get_alive_neighbour_count(row, column, prev_frame);
                if (alive_neighbours < 2 || alive_neighbours > 3) {
                    cells_to_kill.push_back(std::array<int, 2> {row, column});
                    continue;
                }
                if (alive_neighbours = 3 && !current_cell_state) {
                    cells_to_revive.push_back(std::array<int, 2> {row, column});
                }
            }
        }
        kill_cells(cells_to_kill, new_frame);
        revive_cells(cells_to_revive, new_frame);
    }

private:

    int get_alive_neighbour_count(int row, int column, bool (*prev_frame)[16]) {
        int alive_neighbours = 0;
        bool current_cell_state = prev_frame[row][column];
        for (int neighbour_row = row - 1; neighbour_row <= row + 1; neighbour_row++) {
            for (int neighbour_column = column - 1; neighbour_column <= column-1; neighbour_column++) {
                bool neighbour_cell_state = prev_frame[neighbour_row][neighbour_column];
                if (neighbour_cell_state) {
                    alive_neighbours++;
                }
            }
        }
        return alive_neighbours;
    }

    void kill_cells(std::vector<std::array<int, 2>> cells_to_kill, bool (*new_frame)[16]) {
        for (int i = 0; i < cells_to_kill.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_kill[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            new_frame[row][column] = 0;
        };
    }

    void revive_cells(std::vector<std::array<int, 2>> cells_to_revive, bool (*new_frame)[16]) {
        for (int i = 0; i < cells_to_revive.size(); i++) {
            std::array<int, 2> cell_coordinates = cells_to_revive[i];
            int row = cell_coordinates[0];
            int column = cell_coordinates[1];
            new_frame[row][column] = 1;
        };
    }
};