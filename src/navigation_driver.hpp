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
    int cursor_sim_pixel_state;
    const int ACTION_REJECTION_WINDOW = 200; // Reject actions within x ms before a movement (used because action inputs occur before up movements but vice versa for down, right, left)
    unsigned int action_time = 0;
    unsigned int movement_time = 0;
    bool action_triggered = false;

public:
    bool game_paused = false;

    NavigationDriver(bool (*&display_buffer_ptr)[16], bool (*&sim_buffer_ptr)[16]) {
        _display_buffer_ptr = display_buffer_ptr;
        _sim_buffer_ptr = sim_buffer_ptr;
        cursor_sim_pixel_state = _sim_buffer_ptr[0][0];
    }

    void refresh_cursor_position() {
        if (!game_paused) return;

        if (cursor_change) {
            // Ensures that the cursor is lit up every time its position is changed to make visibility clearer 
            cursor_change = false;
            cursor_lit = true;
            last_blink = millis();
        }

        unsigned int current_time = millis();

        if ((current_time - action_time >= ACTION_REJECTION_WINDOW) && (movement_time - action_time >= ACTION_REJECTION_WINDOW) && action_triggered) {
            accept_action();
        }

        if (current_time - last_blink >= 500) {
            last_blink = current_time;
            if (cursor_lit) {
                cursor_lit = false;
            } else {
                cursor_lit = true;
            }
        }

        if (cursor_lit) {
            _display_buffer_ptr[current_row][current_column] = true;
        } else {
            _display_buffer_ptr[current_row][current_column] = false;
        }
    }

    void move_up() {
        if (!game_paused) return;
        prev_row = current_row;
        prev_column = current_column;
        _display_buffer_ptr[prev_row][prev_column] = cursor_sim_pixel_state;
        cursor_change = true;
        movement_time = millis();
        if (current_row < 15) {
            current_row++;
        } else {
            current_row = 0;
        }
        cursor_sim_pixel_state = _sim_buffer_ptr[current_row][current_column];
    }

    void move_down() {
        if (!game_paused) return;
        prev_row = current_row;
        prev_column = current_column;
        _display_buffer_ptr[prev_row][prev_column] = cursor_sim_pixel_state;
        cursor_change = true;
        movement_time = millis();
        if (current_row > 0) {
            current_row--;
        } else {
            current_row = 15;
        }
        cursor_sim_pixel_state = _sim_buffer_ptr[current_row][current_column];
    }

    void move_right() {
        if (!game_paused) return;
        prev_column = current_column;
        prev_row = current_row;
        _display_buffer_ptr[prev_row][prev_column] = cursor_sim_pixel_state;
        cursor_change = true;
        movement_time = millis();
        if (current_column < 15) {
            current_column++;
        } else {
            current_column = 0;
        }
        cursor_sim_pixel_state = _sim_buffer_ptr[current_row][current_column];
    }

    void move_left() {
        if (!game_paused) return;
        prev_column = current_column;
        prev_row = current_row;
        _display_buffer_ptr[prev_row][prev_column] = cursor_sim_pixel_state;
        cursor_change = true;
        movement_time = millis();
        if (current_column > 0) {
            current_column--;
        } else {
            current_column = 15;
        }
        cursor_sim_pixel_state = _sim_buffer_ptr[current_row][current_column];
    }

    void centre_select() {
        if (!game_paused) return;
        
        action_time = millis();
        action_triggered = true;
    }

    void accept_action() {
        action_triggered = false;
        if (_sim_buffer_ptr[current_row][current_column]) {
            _sim_buffer_ptr[current_row][current_column] = false;
        } else {
            _sim_buffer_ptr[current_row][current_column] = true;
        }
    }
};