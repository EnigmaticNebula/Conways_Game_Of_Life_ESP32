#include <Arduino.h>

const int HIGH_SIDE_SERIAL_PIN = 15;
const int HIGH_SIDE_RCLK_PIN = 13;
const int HIGH_SIDE_SRCLK_PIN = 14;
const int LOW_SIDE_SERIAL_PIN = 25;
const int LOW_SIDE_CLK_PIN = 26;
const int LOW_SIDE_LATCH_PIN = 27;

class LedDriver {
    private:
    
    bool (*_display_buffer)[16];

    void high_side_send_bit (boolean bit ) {
        digitalWrite(HIGH_SIDE_SERIAL_PIN, bit);
        digitalWrite(HIGH_SIDE_RCLK_PIN, HIGH);
        digitalWrite(HIGH_SIDE_RCLK_PIN, LOW);
    }

    void low_side_send_bit( boolean bit ) {
        digitalWrite(LOW_SIDE_SERIAL_PIN, bit);
        digitalWrite(LOW_SIDE_CLK_PIN, HIGH);
        digitalWrite(LOW_SIDE_CLK_PIN, LOW);
    }

    void select_row( int row ) {
        /**
         * Counts down from 15 instead as selecting row 0 would require
         * sending 1 on the 16th bit. 
         */
        for (int i = 15; i >= 0; i--) {
            if (i == row) {
                high_side_send_bit(HIGH);
            } else {
                high_side_send_bit(LOW);
            }
        }
    }

    void trigger_latches() {
        digitalWrite(LOW_SIDE_LATCH_PIN, HIGH);
        digitalWrite(HIGH_SIDE_SRCLK_PIN, HIGH);
        digitalWrite(LOW_SIDE_LATCH_PIN, LOW);
        digitalWrite(HIGH_SIDE_SRCLK_PIN, LOW);  
    }

    public:

    LedDriver(bool (*&display_buffer)[16]) {
        _display_buffer = display_buffer;
    }


    void refresh() {
        for (int i = 0; i < 16; i++) {
            select_row(i);
            // Refresh can theoretically be called whilst cell_states is briefly a nullptr whilst swapping sim and display buffer pointers
            if (_display_buffer == nullptr) {
                return;
            }
            bool* row_cell_states = _display_buffer[i];
            for (int j = 0; j < 16; j++) {
                bool cell_state = row_cell_states[j];
                low_side_send_bit(cell_state);
            }
        }
        trigger_latches();
    }
};