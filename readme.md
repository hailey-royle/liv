
# ModifyChain

//normal
A > B > C
A > B1 > B2 > C
A > B1 > D > B2 > C

//delete only
A > B > C
A > B1 > B2 > C

//insert only
A > B > C
A > D > B > C

//delete beginning
A > B > C
A > B1 > C
A > D > B1 > C

# notes to self

- was working of start of insert mode fuctionality, specificaly backspace
    - found bug where ModifyChain always modifys the begining
    - want to move form 0 based liv.cursor and chain lineOffset to 1 based

    - cashe the chrrent line, and write to table if crossing over live
