# ncTodo ~ Terminal Todo List
A minimalist Todo list, based on the *ncAuth* UI.


## Usage
### CLI
*ncTodo* acceps a few command line arguments to allow for automated usage.

`--help`  
Prints the help page & exits.

`--file [file]`  
Specifies a Todo file, which if unspecified reads `./tasks.todo`.  
Each task is defined on a new line - subtasks are defined by tasks indented by `\t` underneath the parent task. Tasks are marked as completed by prefixing an asterisk (`*`) before the task, but after the tabs.


### GUI
In the interactive UI, you will see a list of your Todo tasks. Tasks with subtasks have a progressbar running through them to show how many have been completed.

`k` & `j`, `g` & `G`  
Navigate the Todo list; move up, down, and jump to the start or end. If you have more Todo tasks that can fit on your screen, they will scroll with the selection.

`K` & `J`  
Move the selected task up & down.

`h` & `l`  
Collapse & expand the selected task to view its subtasks.

`o` & `O`  
Create a new task in the currently expanded parent, below or above the selected task. Will call `./edit.sh` for a new description.

`i`  
Create a new task as a subtask of the currently selected one. Will call `./edit.sh` for a new description.

`e`  
Edit the currently selected task. Will call `./edit.sh $current_text` for a new description.

`d` & `D`  
Delete the currently selected task, but only if it is marked as completed. If a task has subtasks you need to hold shift, which will recursively delete all subtasks that are completed.

`x` & `r`, `X` & `R`  
Mark a task as completed & reset a task. If a task has subtasks you need to hold shift, which will recursively mark all subtasks.

`q`  
dies lol


## Building
Dependencies:
- `libncurses`
- Standard C libraries

Then run:  
`gcc src/nctodo.c -lncurses -o nctodo`

### Installing
As with *ncAuth*, I have not specified any installation directory; the tasks list and `edit.sh` are found in the working directory.  
To "install" it, move `edit.sh` and the executable wherever you want to keep your tasks file (such as your home folder), then make a script to `cd` to & call it somewhere in your `$PATH`.
