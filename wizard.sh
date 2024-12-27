#!/bin/bash

DIALOG_CANCEL=1
DIALOG_ESC=255
HEIGHT=0
WIDTH=0

if command -v dialog &> /dev/null; then
	DIAL="dialog"
elif command -v whiptail &> /dev/null; then
	DIAL="whiptail"
else
	echo "No dialog-compatible program found, please install one"
	exit 1
fi

isroot() {
	if [ "$EUID" -ne 0 ]; then
		display_result "Please run as root" "This action requires root privileges"
		return 1
	fi
	return 0
}

DIALOG() {
	$DIAL --clear --backtitle "kri Installation Wizard" "$@"
}

display_result() {
	DIALOG --title "$1" \
	--msgbox "$2" $HEIGHT $WIDTH
}

KEY_SAVE="ctrl('S')"
KEY_EXIT="ctrl('X')"
KEY_INFO="'i'"
KEY_REFRESH="ctrl('R')"
KEY_HOME="ctrl('A')"
KEY_END="ctrl('E')"

key_config() {
	exec 3>&1 # open stdout fd
	OUTPUT=$(DIALOG --title "Edit Keybindings" \
	--form "Edit the fields" $HEIGHT $WIDTH 0 \
	"Save"		1 1 "$KEY_SAVE"		1 15 15 0 \
	"Exit"		2 1 "$KEY_EXIT"		2 15 15 0 \
	"Home"		3 1 "$KEY_HOME"		3 15 15 0 \
	"End"		4 1 "$KEY_END"		4 15 15 0 \
	"Info"		5 1 "$KEY_INFO"		5 15 15 0 \
	"Refresh"	6 1 "$KEY_REFRESH"	6 15 15 0 \
	2>&1 1>&3)
	exit_status=$?
	case $exit_status in
	"$DIALOG_CANCEL")
		clear
		return
	    	;;
	"$DIALOG_ESC")
		clear
		return
		;;
	esac
	exec 3>&- # delete fd

	outputarray=($OUTPUT) # convert to array
	# Replace the key* with the output of dialog
	sed -i "8s/$KEY_SAVE/${outputarray[0]}/" headers/keybindings.h 
	sed -i "9s/$KEY_EXIT/${outputarray[1]}/" headers/keybindings.h 
	sed -i "16s/$KEY_HOME/${outputarray[2]}/" headers/keybindings.h 
	sed -i "17s/$KEY_END/${outputarray[3]}/" headers/keybindings.h 
	sed -i "18s/$KEY_INFO/${outputarray[4]}/" headers/keybindings.h 
	sed -i "19s/$KEY_REFRESH/${outputarray[5]}/" headers/keybindings.h
}

color_config() {
 	mapfile -t colors <  <(sed "s/.*COLOR_//g" screen/highlight.cpp | head -n 17 | tail -n 6)

	exec 3>&1 # open stdout fd
	OUTPUT=$(DIALOG --title "Edit Colors" \
	--form "Edit the fields" $HEIGHT $WIDTH 0 \
	"#include/define"	1 1 "${colors[0]}"	1 20 20 0 \
	"Comments"		2 1 "${colors[1]}"	2 20 20 0 \
	"Types"			3 1 "${colors[2]}"	3 20 20 0 \
	"Operators"		4 1 "${colors[3]}"	4 20 20 0 \
	"if/while"		5 1 "${colors[4]}"	5 20 20 0 \
	"Strings"		6 1 "${colors[5]}"	6 20 20 0 \
	2>&1 1>&3)
	exit_status=$?
	case $exit_status in
	"$DIALOG_CANCEL")
		clear
		return
	    	;;
	"$DIALOG_ESC")
		clear
		return
		;;
	esac
	exec 3>&- # delete fd

	outputarray=($OUTPUT) # convert to array
	# Replace the key* with the output of dialog
	sed -i "12s/${colors[0]}/${outputarray[0]}/" screen/highlight.cpp 
	sed -i "13s/${colors[1]}/${outputarray[1]}/" screen/highlight.cpp 
	sed -i "14s/${colors[2]}/${outputarray[2]}/" screen/highlight.cpp 
	sed -i "15s/${colors[3]}/${outputarray[3]}/" screen/highlight.cpp 
	sed -i "16s/${colors[4]}/${outputarray[4]}/" screen/highlight.cpp 
	sed -i "17s/${colors[5]}/${outputarray[5]}/" screen/highlight.cpp
}

config_dialog() {
	high_st="Disable"
	tmp="$(cat Makefile | head -n 16 | tail -n 1)"
	if ! grep -q "\-DHIGHLIGHT" <<< "$tmp"; then
		high_st="Enable"
	fi

	while true; do
		exec 3>&1 # open stdout fd
		selection=$(DIALOG --title "$1" \
		    --menu "Please select:" $HEIGHT $WIDTH 0 \
		    "1" "Edit Keybindings" \
		    "2" "Install dependencies" \
		    "3" "$high_st syntax highlighting" \
		    "4" "Change colors of syntax highlighting" \
		2>&1 1>&3) # redirect output streams
		exit_status=$?
		exec 3>&- # delete fd
		case $exit_status in
		"$DIALOG_CANCEL")
			clear
			break
			;;
		"$DIALOG_ESC")
			clear
			break
			;;
		esac
		case $selection in
		1 )
			if [ $DIAL = "dialog" ]; then
				key_config
			else
				display_result "dialog not installed, but you can still edit keybindings.h"
			fi
			;;
		2 )
			clear
			if isroot; then
				echo "#### Installing Dependencies"
				if command -v apt &> /dev/null; then
					apt install libncurses-dev gcc make dialog
				elif command -v dnf &> /dev/null; then
					dnf install ncurses-devel gcc make dialog
				elif command -v pacman &> /dev/null; then
					pacman -S --needed ncurses gcc make dialog
				fi
			fi
			sleep 1
		   	;;
		3 )
			if [ $high_st = "Enable" ]; then
				sed -i 's/CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -lncursesw/CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -DHIGHLIGHT -lncursesw/g' Makefile
				high_st="Disable"
			else
				sed -i 's/CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -DHIGHLIGHT -lncursesw/CXXFLAGS = -Wall -Wextra -pedantic $(OPTIM) -lncursesw/g' Makefile
				high_st="Enable"
			fi
			;;
		4 )
			if [ $DIAL = "dialog" ]; then
				color_config
			else
				display_result "dialog not installed, but you can still edit utils/highlight.cpp lines 23-28"
			fi
			;;
		esac
    	done
}

while true; do
    	exec 3>&1
    	selection=$(DIALOG --title "Menu" \
		--menu "Please select:" $HEIGHT $WIDTH 0 \
		"1" "Configure" \
		"2" "Compile" \
		"3" "Install" \
		"4" "Uninstall" \
    	2>&1 1>&3)
    	exit_status=$?
    	exec 3>&-
    	case $exit_status in
		"$DIALOG_CANCEL")
		    clear
		    echo "Program terminated."
		    exit
		;;
		"$DIALOG_ESC")
		    clear
		    echo "Program aborted." >&2
		    exit 1
		;;
    	esac
    	case $selection in
	1 )
		config_dialog "Configuration Options"
		;;
	2 )
		if make build -j$(nproc) 2>&1 >/dev/null | grep -q Error; then
			display_result "Failed to build kri" "Make sure to report the output of make at: https://github.com/ikozyris/kri/issues"
	    	else
			display_result "Succesfully built kri" "Now you can install!"
		fi
		;;
	3 )
		if [ -d ~/.local/bin ]; then
			cp kri ~/.local/bin/
			display_result "Installed kri" "Installed in ~/.local/bin"
		else
			if isroot; then
				cp kri /usr/bin/
				display_result "Installed kri" "Installed in /usr/bin"
			fi
		fi
		;;
	4 )
		if [ -f ~/.local/bin/kri ]; then
			rm ~/.local/bin/kri
			display_result "We are sorry to see you go." "Uninstalled from ~/.local/bin"
		else
			if isroot; then
				rm /usr/bin/kri
				display_result "We are sorry to see you go." "Uninstalled from /usr/bin"
			fi
		fi
		;;
    	esac
done
