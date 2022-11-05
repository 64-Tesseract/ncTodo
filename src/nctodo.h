/* Config area */

// ncurses UI primary colour
// 1: Red
// 2: Green
// 3: Yellow
// 4: Blue
// 5: Magenta
// 6: Cyan
#define UI_COLOUR 1

// Subtask maximum depth
#define TASK_DEPTH 16

/* Macros & structs, no touchie */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MINMAX(val, min, max) (MIN(MAX((val), (min)), (max)))

#define WHITEF 0
#define COLOURF 1
#define WHITEB 2
#define COLOURB 3

struct Task {
    char desc[256];  // Text description
    float done;  // Percentage done of task
    int highlight;  // Selected subtask
    int subtaskc;  // Subtask count
    int subtaskmc;  // Size of list (max count)
    struct Task** subtasks;  // List of subtask addresses
};
