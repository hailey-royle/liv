
# Liv!

Liv is a text editor I have made for my personal idiosyncrasys and fun. 
It is currently working!  

# Usage

Liv has two modes, insert and command.  
On start, liv is in command mode.  
Enter insert mode by pressing 'i'(or 'a' or 'o'), exit by pressing 'escape'.  

## Commands

All commands follow the pattern of, [count - command - motion]  

### implemented

**Count**  
- *n* - do thing x times

**Command**  
- q - quit
- w - write
- i - insert
- a - append
- o - open
- g - go to line

**Motion**  
- h - cursor left
- l - cursor right
- j - line down
- k - line up
- b - word left
- e - word right
- m - paragraph up
- n - paragraph down
- x - line start
- v - line end
- t - file start
- z - file end

### planned

- d - delete
- c - change
- y - yeet
- p - paste
- f - find
- u - undo
- r - redo
- s - subsitute
- ? - repeat command
- ? - around [word | line | paragraph | file]
