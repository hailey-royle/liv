# todo

## functions

EnterInsertMode()
    SplitPiece()
    InsertPiece()

Insert(key)
    if key is \b 
        if length is 0
            RemovePiece()
            ShiftPiece(-1)
        else 
            offset--
    else
        Append(key)
        length++

# done

## functions

GetLineRelitive()
    FindLineNext()
    FindLinePrevious()
    GetLine()
MoveLineRelitive()
    FindLineNext()
    FindLinePrevious()
CursorLeft()
CursorRight()
