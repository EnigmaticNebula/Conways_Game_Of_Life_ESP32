#include <Arduino.h>

class NavigationDriver {
private:
    bool (*_display_buffer_ptr)[16];
    bool (*_sim_buffer_ptr)[16];
    unsigned int current_row = 0;
    unsigned int current_column = 0;
    unsigned int prev_row = 0;
    unsigned int prev_column = 0;
    unsigned int last_blink = 0;
    bool cursor_lit = true;
    bool cursor_change = false;

public:
    NavigationDriver(bool (*&display_buffer_ptr)[16], bool (*&sim_buffer_ptr)[16]) {
        _display_buffer_ptr = display_buffer_ptr;
        _sim_buffer_ptr = sim_buffer_ptr;
    }


    void refresh_cursor_position() {
            // The led where at the cursor's previous position must be turned off unless the cell at that position is alive.
            // There is no need to check the sim buffer as any manual changes will be applied on the next display refresh
            if (!_display_buffer_ptr[prev_row][prev_column] && cursor_change) {
                _display_buffer_ptr[prev_row][prev_column] = false;
                
                // Ensures that the cursor is lit up every time its position is changed to make visibility clearer 
                cursor_change = false;
                cursor_lit = true;
                last_blink = millis();
            }

            unsigned int current_time = millis();
            if (current_time - last_blink >= 2000) {
                last_blink = current_time;
                if (cursor_lit) {
                    cursor_lit = false;
                } else {
                    cursor_lit = true;
                }
            }

            if (cursor_lit) {
                _display_buffer_ptr[current_row][prev_column] = true;
            } else {
                _display_buffer_ptr[current_row][prev_column] = false;
            }
        }

    void move_up() {
        if (current_row < 15) {
            prev_row = current_row;
            current_row++;
            cursor_change = true;
        }
    }

    void move_down() {
        if (current_row > 0) {
            prev_row = current_row;
            current_row--;
            cursor_change = true;
        }
    }

    void move_right() {
        if (current_column < 15) {
            prev_column = current_column;
            current_column++;
            cursor_change = true;
        }
    }

    void move_left() {
        if (current_column > 0) {
            prev_column = current_column;
            current_column--;
            cursor_change = true;
        }
    }

    void centre_select() {
        if (_sim_buffer_ptr[current_row][current_column]) {
            _sim_buffer_ptr[current_row][current_column] = false;
        } else {
            _sim_buffer_ptr[current_row][current_column] = true;
        }
    }

};