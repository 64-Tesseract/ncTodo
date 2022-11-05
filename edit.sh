#!/bin/sh

# $1: Mode (1: Edit, 2: Read)
# $2: Existing text

if [ "$1" = "1" ]; then
    # Create temporary file with task description
    if printf "%s" "$2" > /tmp/nctodotask; then
        # If file created, open in an editor
        if [ "$EDITOR" != "" ]; then
            if [ "$EDITOR" = "nano" ]; then
                # nano users deverse purgatory
                sxmo_terminal.sh vi /tmp/nctodotask > /dev/null
            else
                sxmo_terminal.sh $EDITOR /tmp/nctodotask > /dev/null
            fi
        else
            # Replace with your own editor
            sxmo_terminal.sh nvim /tmp/nctodotask > /dev/null
        fi

        if [ "$(printf "%s" "$(cat /tmp/nctodotask)" | tr -d "\n" | tr -d " ")" = "" ]; then
            exit 1
        else
            exit 0
        fi

    else
        exit 1
    fi

elif [ "$1" = "2" ]; then
    cat /tmp/nctodotask | tr "\n" " "

    # Remove temp file
    rm /tmp/nctodotask
fi
