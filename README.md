# Operating-Systems-Architecture-Semester-Project
by Matthew Fortier and Anthony Taylor

This is the semester long project to build a simple shell using the FAT12
file system for CSI-385 Operating Systems Architecture

# Commands Available
cat <filename>          - Prints the selected file
cd <path>               - Changes to the selected directory
df                      - Prints the used blocks of memory
ls <path>               - Lists all files in current or given directory
mkdir <directory-name>  - Creates directory
pbs                     - Prints the boot sector details
pfe <start> <end>       - Prints the fat entries between the given range
pwd                     - Prints the current working directory
rm <filename>           - Removes given filename
rmdir <directory>       - Removes given directory
touch <filename>        - Creates file with given name

# Installation and Execution
1. Run 'make' in the 'final_project' directory
2. Execute './shell' in the same directory
