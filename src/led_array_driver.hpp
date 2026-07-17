#include <Arduino.h>

class LedDriver {
    private:
    
    bool** cell_states;

    void send_bit( boolean bit ) {
        digitalWrite(14, bit);
        digitalWrite(26, HIGH);
        digitalWrite(26, LOW);
    }

    void select_row( int row ) {
        /**
         * Counts down from 15 instead as selecting row 0 would require
         * sending 1 on the 16th bit. 
         */
        for (int i = 15; i >= 0; i--) {
            if (i == row) {
                send_bit(HIGH);
            } else {
                send_bit(LOW);
            }
        }
    }

    void refresh() {
        for (int i = 0; i < 16; i++) {
            select_row(i);
            // Refresh can be called whilst cell_states is briefly a nullptr
            if (cell_states == nullptr) {
                return;
            }
            bool* row_cell_states = cell_states[i];
            for (int j = 0; j < 16; j++) {
                bool cell_state = row_cell_states[j];
                send_bit(cell_state);
            }
        }
    }

    public:

    void update_cell_states( bool** new_cell_states ) {
        // Deallocate old cell state array
        if (cell_states != nullptr) {
            delete[] cell_states;
            cell_states = nullptr; // Ensures cell_states isn't a hanging pointer
        }
        **cell_states = new_cell_states;
    };

    /**
     * Keep static as FreeRTOS is written in C and expects static C++
     * functions (they have the same behaivour as C functions)
     */
    static void array_begin(void* pvParameters) {
        LedDriver* obj = static_cast<LedDriver*>(pvParameters);
        if (obj != nullptr) {
            for(;;) {
                obj->refresh();
                vTaskDelay(1); // Prevents task watchdog timer (TWDT) panics
            }
        }
        vTaskDelete(NULL);
    }
};