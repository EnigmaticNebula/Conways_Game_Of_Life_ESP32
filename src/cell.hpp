class Cell {
public:
    int row;
    int column;
    bool alive = false;
    Cell(int row, int column) : row(row), column(column) {}
    Cell() {}

    void revive_cell() {
        alive = true;
    }
    
    void kill_cell() {
        alive = false;
    }
};