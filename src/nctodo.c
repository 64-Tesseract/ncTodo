#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include "nctodo.h"


struct Task* new_task (char* desc) {
    struct Task* t = malloc(sizeof (struct Task));
    strcpy(t->desc, desc);
    t->done = 0;
    t->subtaskc = 0;
    t->subtaskmc = 0;
    t->subtasks = NULL;
    return t;
}


void del_newline (char* string, int len) {
    for (int c = 0; c < len; c++) {
        if (string[c] == '\n') {
            string[c] = 0;
            return;
        }
    }
}


void add_task (struct Task* parent, struct Task* subtask, int pos) {
    // printf("Parent in function: %s\n", parent->desc);
    // printf("Subtask to add: %s\n", subtask->desc);
    // Ensure subtask list has enough spaces for a new task
    if (parent->subtaskc == 0) {
        // Start with 4 slots
        parent->subtaskmc = 4;
        // printf("Allocating array in %s\n", parent->desc);
        parent->subtasks = malloc(4 * sizeof (struct Task*));
        // printf("Allocated\n");

    } else {
        // If list is too small, reallocate it
        if (parent->subtaskc == parent->subtaskmc) {
            // printf("Reallocating array (%d / %d)\n", parent->subtaskc, parent->subtaskmc);
            parent->subtaskmc *= 2;
            parent->subtasks = realloc(parent->subtasks, parent->subtaskmc * sizeof (struct Task*));
        }
    }

    // Add the task
    // printf("Adding\n");
    if (pos == -1)
        pos = parent->subtaskc;

    for (int s = parent->subtaskc - 1; s >= pos; s--) {
        parent->subtasks[s + 1] = parent->subtasks[s];
    }

    parent->subtasks[pos] = subtask;
    parent->subtaskc = parent->subtaskc + 1;
    // printf("%d subtasks\n", parent->subtaskc);
}


void backshift_tasks (struct Task* parent, int index) {
    for (int s = index; s < parent->subtaskc - 1; s++)
        parent->subtasks[s] = parent->subtasks[s + 1];

    parent->subtaskc--;
}


void del_task (struct Task* parent, int index) {
    struct Task* task = parent->subtasks[index];

    for (int s = task->subtaskc - 1; s >= 0; s--)
        del_task(task, s);

    if (task->subtaskc == 0 && task->done == 1) {
        free(task);
        backshift_tasks(parent, index);
    }
}


void mark_recurse (struct Task* task, bool done) {
    task->done = done ? 1 : 0;
    for (int s = 0; s < task->subtaskc; s++)
        mark_recurse(task->subtasks[s], done);
}


void print_task (struct Task* task, int indent) {
    for (int i = 0; i < indent; i++)
        printf("    ");

    printf("%s",  task->desc);
    if (task->subtaskc != 0)
        printf(" (%.0f%%)", task->done * 100);
    else if (task->done == 1)
        printf(" (Done)");
    printf("\n");

    for (int s = 0; s < task->subtaskc; s++)
        print_task(task->subtasks[s], indent + 1);
}


bool edit_task (struct Task* task) {
    char command[1024];
    snprintf(command, 1024, "%s/edit.sh 1 '%s'", getcwd(NULL, 256), task->desc);

    if (system(command) == 0) {
        snprintf(command, 128, "%s/edit.sh 2", getcwd(NULL, 256));
        FILE* edited_desc = popen(command, "r");

        fgets(task->desc, 256, edited_desc);
        pclose(edited_desc);

        return false;
    }

    return true;
}


int read_tasks (struct Task* root_task, char* path) {
    char line[257 + TASK_DEPTH];
    FILE* file = fopen(path, "r");

    if (file != NULL) {
        // List of references to parent tasks on each level
        struct Task* last_parents[TASK_DEPTH];
        last_parents[0] = root_task;
        int last_parentc = 0;

        while (fgets(line, 257 + TASK_DEPTH, file)) {
            // Count indentation
            int tabs = 0;
            while (true) {
                if (line[tabs] == '\t')
                    tabs++;
                else
                    break;
            }
            tabs = MIN(tabs, last_parentc);

            // Check if task is marked as done
            bool done = false;
            if (line[tabs] == '*')
                done = true;

            // Extract description without tabs & `*`
            char desc[256];
            strncpy(desc, &line[tabs + done], 256 - tabs - done);
            del_newline(desc, 256);
            // printf("%d %d: %s\n", tabs, done, desc);

            struct Task* t = new_task(desc);
            t->done = done ? 1.0 : 0.0;
            // printf("Task address: %x  ", t);
            // printf("Subtasks: %d  ", t->subtaskc);
            // printf("Desc: %s\n", t->desc);
            // printf("\n");
            // printf("\n");
            add_task(last_parents[tabs], t, -1);
            last_parents[tabs + 1] = t;
            last_parentc = tabs + 1;

            // printf("%d\n", last_parents[0]->subtaskc);
        }

        fclose(file);

    } else {
        printf("No such tasks file\n");
    }
}


void write_append_task (FILE* file, struct Task* task, int indent) {
    for (int i = 0; i < indent; i++)
        fputc('\t', file);

    if (task->subtaskc == 0 && task->done == 1)
        fputc('*', file);

    fputs(task->desc, file);
    fputc('\n', file);

    for (int s = 0; s < task->subtaskc; s++)
        write_append_task(file, task->subtasks[s], indent + 1);
}


void write_tasks (struct Task* root_task, char* path) {
    FILE* file = fopen(path, "w");

    for (int s = 0; s < root_task->subtaskc; s++)
        write_append_task(file, root_task->subtasks[s], 0);

    fclose(file);
}


int swap_tasks (struct Task* parent, int index, int dir) {
    index = MINMAX(index, 0, parent->subtaskc - 1);
    int index2 = MINMAX(index + dir, 0, parent->subtaskc - 1);
    struct Task* temp_task = parent->subtasks[index];
    parent->subtasks[index] = parent->subtasks[index2];
    parent->subtasks[index2] = temp_task;

    return index2;
}


int calc_scroll (int select, int count, int height) {
    // If list fits in window, put them up top
    if (count <= height) return 0;

    // By default, the 1st item (scroll) should be half the size of the view area
    // above the selection, so the selection is in the middle of the screen
    int scroll = select - height / 2;
    // Limit to 1st item being on top
    if (scroll < 0) scroll = 0;
    // Limit last item to being on bottom of view area
    if (scroll > count - height) scroll = count - height;
    return scroll;
}


float calculate_completion (struct Task* task) {
    if (task->subtaskc != 0) {
        float total_completion = 0;

        for (int s = 0; s < task->subtaskc; s++) {
            total_completion += calculate_completion(task->subtasks[s]);
        }

        task->done = total_completion / task->subtaskc;
    }

    return task->done;
}


char* shorten_string (char* string) {
    if (strlen(string) > COLS - 2) {
        char* new_string = malloc(512);
        strncpy(new_string, string, COLS - 2);
        new_string[COLS - 5] = '.';
        new_string[COLS - 4] = '.';
        new_string[COLS - 3] = '.';
        new_string[COLS - 2] = 0;

        return new_string;
    }

    return string;
}


void define_colour (int colour) {
    init_pair(WHITEB, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOURF, colour, COLOR_BLACK);
    init_pair(COLOURB, COLOR_BLACK, colour);
}


void set_colour () {
    // (int) -> (int, but with extra steps)
    switch (UI_COLOUR) {
        default:
        case 1:
            define_colour(COLOR_RED);
            break;
        case 2:
            define_colour(COLOR_GREEN);
            break;
        case 3:
            define_colour(COLOR_YELLOW);
            break;
        case 4:
            define_colour(COLOR_BLUE);
            break;
        case 5:
            define_colour(COLOR_MAGENTA);
            break;
        case 6:
            define_colour(COLOR_CYAN);
            break;
    }
}

int main (int argc, char *argv[]) {
    char file[256];
    snprintf(file, 256, "%s/tasks.todo", getcwd(NULL, 256));
    struct Task* root_task;
    bool print_mode = false;


    // Literally half of the code is just argument parsing
    for (int a = 1; a < argc; a++) {
        if (strcmp(argv[a], "--help") == 0 || strcmp(argv[a], "-h") == 0) {
            printf("   ncTodo ~ Terminal Todo List by 64_Tesseract\n");
            printf("--help, -h              Prints this list & exits\n");
            printf("--file [file]           Specifies a tasks list file, by default `./tasks.todo`\n");
            printf("--print                 Prints all tasks instead of running in interactive mode\n");
            return 0;

        } else if (strcmp(argv[a], "--file") == 0) {
            if (argc <= a + 1) {
                printf("Missing a file path\n");
                exit(1);
            }

            char* new_file = argv[++a];
            strcpy(file, new_file);

        } else if (strcmp(argv[a], "--print") == 0) {
            print_mode = true;
            a++;

        } else {
            printf("Unknown argument \"%s\", see --help\n", argv[a]);
            exit(1);
        }
    }

    char* list_name = strrchr(file, '/');
    root_task = new_task(list_name ? list_name + 1 : file);
    read_tasks(root_task, file);
    calculate_completion(root_task);

    // Just calculate all tasks once, print, & die
    if (print_mode) {
        print_task(root_task, 0);
        exit(0);
    }


    // 2nd half of code: ncurses GUI
    initscr();
    start_color();
    noecho();
    curs_set(FALSE);
    // nodelay(stdscr, TRUE);

    set_colour();

    // Why don't I define these with the other vars? Too much scrolling
    // How many tasks have been expanded
    int selection_depth = 0;
    // Scroll value
    int scroll = 0;
    // Last task in the selection chain
    struct Task* selected_task = root_task;


    bool gui_running = true;
    while (gui_running) {
        clear();

        if (selected_task->subtaskc != 0) {
            for (int a = 0; a < (MIN(LINES - 1, selected_task->subtaskc - scroll)); a++) {
                int aa = a + scroll;
                struct Task* subtask = selected_task->subtasks[aa];
                bool this_selected = aa == selected_task->highlight;

                if (subtask->subtaskc != 0) {
                    // Calculate progress bar
                    int pbar = (int)(subtask->done * (COLS - 1) + 1);

                    char text[512];
                    snprintf(text, 512, " %-512s", shorten_string(subtask->desc));

                    // Split left & right sides of text within progress bar
                    char left[512];
                    strncpy(left, text, pbar);
                    left[pbar] = 0;

                    char right[512];
                    snprintf(right, 511, "%s", &text[pbar]);

                    // Colour left side & uncolour right
                    attrset(COLOR_PAIR(this_selected ? COLOURB : WHITEB));
                    mvprintw(a, 0, "%s", left);
                    attrset(COLOR_PAIR(this_selected ? COLOURF : WHITEF));
                    mvprintw(a, pbar, "%s", right);

                } else {
                    attrset(COLOR_PAIR(this_selected ? COLOURF : WHITEF));
                    if (subtask->done == 1) {
                        attron(A_DIM);
                        attron(A_ITALIC);
                    }
                    mvprintw(a, 1, "%s", subtask->desc);
                }
            }

        } else {
            // If no codes, show error in middle of screen
            attrset(COLOR_PAIR(WHITEF));
            attron(A_BLINK);
            mvprintw((LINES - 1) / 2, COLS / 2 - 6, "- No tasks -");
        }

        // Progress bar of title, centered
        int pbar = (int)(selected_task->done * (COLS - 1) + 1) / 2;
        int space_len = MAX(1, (int)((COLS - strlen(selected_task->desc)) / 2.0));

        char text[1024];
        snprintf(text, 1024, "%*s%s%*s", space_len, "", shorten_string(selected_task->desc), space_len, "");

        // Split left & right sides of text within progress bar
        char left1[512];
        strncpy(left1, text, COLS / 2 - pbar);
        left1[COLS / 2 - pbar] = 0;
        char left2[512];
        strncpy(left2, &text[COLS / 2 - pbar], pbar);
        left2[pbar] = 0;

        char right1[512];
        strncpy(right1, &text[COLS / 2], pbar);
        right1[pbar] = 0;
        char right2[512];
        strncpy(right2, &text[COLS / 2 + pbar], COLS / 2 - pbar);
        right2[COLS / 2 - pbar] = 0;

        // Colour center & uncolour outside
        attrset(COLOR_PAIR(COLOURF));
        attron(A_BOLD);
        attron(A_ITALIC);
        mvprintw(LINES - 2, 0, "%s", left1);
        mvprintw(LINES - 2, COLS / 2 + pbar, "%s", right2);
        attrset(COLOR_PAIR(COLOURB));
        attron(A_BOLD);
        attron(A_ITALIC);
        mvprintw(LINES - 2, COLS / 2 - pbar, "%s", left2);
        mvprintw(LINES - 2, COLS / 2, "%s", right1);

        attrset(COLOR_PAIR(COLOURF));
        attron(A_BOLD);
        // Show controls at bottom of screen, just the hotkeys if terminal's too narrow
        if (COLS >= 44) {
            mvprintw(LINES - 1, 0, "[KJ]^/v");
            mvprintw(LINES - 1, COLS / 4 - 3, "[HL]</>");
            mvprintw(LINES - 1, COLS / 2 - 6, "[IOD]Add/Del");
            mvprintw(LINES - 1, COLS * 3 / 4 - 4, "[XR]Mark");
            mvprintw(LINES - 1, COLS - 6, "[E]dit");
        } else {
            mvprintw(LINES - 1, 0, "[KJ]");
            mvprintw(LINES - 1, COLS / 4 - 1, "[HL]");
            mvprintw(LINES - 1, COLS / 2 - 2, "[IOD]");
            mvprintw(LINES - 1, COLS * 3 / 4 - 2, "[XR]");
            mvprintw(LINES - 1, COLS - 3, "[E]");
        }
        attroff(A_BOLD);

        refresh();
        // usleep(0x10000);
        struct Task* insert_task;
        struct Task* highlighted_task = selected_task->subtaskc != 0 ? selected_task->subtasks[selected_task->highlight] : NULL;
        switch (getch()) {
            // Change selection
            case 'k':
                selected_task->highlight--;
                break;
            case 'j':
                selected_task->highlight++;
                break;
            case 'g':
                selected_task->highlight = 0;
                break;
            case 'G':
                selected_task->highlight = selected_task->subtaskc - 1;
                break;

            case 'K':
                if (highlighted_task)
                    selected_task->highlight = swap_tasks(selected_task, selected_task->highlight, -1);
                break;
            case 'J':
                if (highlighted_task)
                    selected_task->highlight = swap_tasks(selected_task, selected_task->highlight, 1);
                break;

            // Expand or collapse task
            case 'l':
                if (highlighted_task && highlighted_task->subtaskc != 0) {
                    selected_task = highlighted_task;
                    selection_depth++;
                }
                break;
            case 'h':
                if (selection_depth != 0) {
                    selected_task = root_task;
                    selection_depth--;
                    for (int s = 0; s < selection_depth; s++)
                        selected_task = selected_task->subtasks[selected_task->highlight];
                }
                break;

            // Mark task
            case 'x':
                if (highlighted_task && highlighted_task->subtaskc == 0)
                    highlighted_task->done = 1;
                calculate_completion(root_task);
                break;
            case 'r':
                if (highlighted_task && highlighted_task->subtaskc == 0)
                    highlighted_task->done = 0;
                calculate_completion(root_task);
                break;
            case 'X':
                if (highlighted_task && highlighted_task->subtaskc != 0)
                    mark_recurse(highlighted_task, true);
                calculate_completion(root_task);
                break;
            case 'R':
                if (highlighted_task && highlighted_task->subtaskc != 0)
                    mark_recurse(highlighted_task, false);
                calculate_completion(root_task);
                break;

            // Edit, append, & insert new task
            case 'e':
                if (highlighted_task && edit_task(highlighted_task) && highlighted_task->subtaskc == 0) {
                    highlighted_task->done = 1;
                    del_task(selected_task, selected_task->highlight);
                    calculate_completion(root_task);
                }
                break;
            case 'o':
                insert_task = new_task("New task");
                add_task(selected_task, insert_task, selected_task->subtaskc == 0 ? selected_task->highlight : ++selected_task->highlight);
                edit_task(insert_task);
                calculate_completion(root_task);
                break;
            case 'O':
                insert_task = new_task("New task");
                add_task(selected_task, insert_task, selected_task->highlight);
                edit_task(insert_task);
                calculate_completion(root_task);
                break;
            case 'i':
                if (highlighted_task) {
                    selected_task = highlighted_task;
                    selection_depth++;

                    insert_task = new_task("New task");
                    add_task(selected_task, insert_task, selected_task->highlight);
                    edit_task(insert_task);
                    calculate_completion(root_task);
                }
                break;

            // Delete task
            case 'd':
                if (highlighted_task && highlighted_task->subtaskc == 0) {
                    del_task(selected_task, selected_task->highlight);
                    calculate_completion(root_task);
                }
                break;
            case 'D':
                if (highlighted_task && highlighted_task->subtaskc != 0) {
                    del_task(selected_task, selected_task->highlight);
                    calculate_completion(root_task);
                }
                break;

            // die lol
            case 'q':
                gui_running = FALSE;
                break;
        }
        selected_task->highlight = MINMAX(selected_task->highlight, 0, selected_task->subtaskc - 1);
        scroll = calc_scroll(selected_task->highlight, selected_task->subtaskc, LINES - 1);
    }

    write_tasks(root_task, file);
    endwin();
}

