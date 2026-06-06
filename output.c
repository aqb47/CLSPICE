#include <stdio.h>
#include "matrix.h"
#include "circuit.h"

void format_result(Matrix result, Circuit circuit) {
    // Show node voltages
    printf("\n=== NODE VOLTAGES ===\n");

    for (int i = 0; i < circuit.node_number - 1; i++) {
        printf("v%i = %lf V\n", i + 1, result.data[i][0]);
    }

    printf("\n");

    // Show branch currents
    printf("\n=== BRANCH CURRENTS ===\n");

    for (int i = 0; i < circuit.elements.size; i++) {
        Element element = circuit.elements.element_array[i];

        // If an element is a resistor output that branch current
        if (element.type == 'R') {
            double resistance = element.value;

            // If a node isn't ground we'll get it's voltage from the result matrix, otherwise it's 0V
            double V_pos = element.node_pos > 0? result.data[element.node_pos - 1][0] : 0;
            double V_neg = element.node_neg > 0? result.data[element.node_neg - 1][0] : 0;

            // Calculate branch current by passive sign convention (+ to -)
            double branch_current = (V_pos - V_neg) / resistance;

            // Print it
            printf("i(%s) = %lf A\n", element.name, branch_current);
        }
    }

    printf("\n");

    // If circuit uses voltage source, show them
    if (circuit.voltage_source_number > 0) {
        printf("\n=== INDEPENDENT & DEPENDENT VOLTAGE SOURCE CURRENTS ===\n");

        for (int i = 0, voltage_source_count = 0; voltage_source_count < circuit.voltage_source_number; i++) {
            Element element = circuit.elements.element_array[i];

            // If we find a voltage source output it's current
            if (element.type == 'V' || element.type == 'E' || element.type == 'H') {
                double current_through_voltage_source = result.data[circuit.node_number - 1 + voltage_source_count][0];

                // Print voltage source current
                printf("I(%s) = %lf A\n", element.name, current_through_voltage_source);

                // Increment voltage source counter
                voltage_source_count += 1;
            }
        }
    }

    printf("\n");

    // Pause execution
    getchar();
}