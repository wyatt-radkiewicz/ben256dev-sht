#!/bin/bash

sudo mkdir -p /usr/share/man/man1/
sudo cp man/*.1 /usr/share/man/man1/;
# Install to different directory (user's .local?)
if [ $? -ne 0 ]; then
	mkdir -p ~/.local/share/man/man1
	cp man/*.1 ~/.local/share/man/man1/;
	echo installing man 1 to ~/.local/share/man/
else
	echo installed man 1 to /usr/share/man/
fi

sudo mkdir -p /usr/share/man/man3/
sudo cp man/*.3 /usr/share/man/man3/;
# Install to different directory (user's .local?)
if [ $? -ne 0 ]; then
	mkdir -p ~/.local/share/man/man3
	cp man/*.3 ~/.local/share/man/man3/;
	echo installing man 3 to ~/.local/share/man/
else
	echo installed man 3 to /usr/share/man/
fi

echo "sht man pages installed successfully"
mandb
