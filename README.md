# Assignment One - C Shell
## Amardeep Singh

Compile: 
make

Run:
./myShell

Discription:
This program is a simple copy of Bash.

Assumptions:
    All special commands (<, >, |, &) will have a space before and after.
    No command ends with a <, >, |
    The setenv and getenv commands can be used as stated in the course Discord.
    The history commands that are being tested are 
        history
        history n (where n is any positive integer)
        history -c
    Environment variables will always be exported in a proper format. 
    If the PATH is included in the profile file then it must include a path to a bin so that the basic commands work.
        (TA insured that this would happen and also stated that my export and echo commands are working correctly.)
    myShell will start in the directory it was run in.
