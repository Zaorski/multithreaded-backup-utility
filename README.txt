OWNER: Thomas Zaorski
EMAIL: Zaorst@rpi.edu

This program will backup all files in the current working directory to a hidden directory. Program can also restore all of the files in the hidden directory to the CWD if the optional argument "-r" is included.

-lpthread is required for compilation using gcc 

Backup files are stored in the hidden folder I create (if it does not already exist) called ".mybackup"
