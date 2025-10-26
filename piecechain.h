struct piece {
    int next;
    int prev;
    int offset;
    int length;
};

struct chain {
    struct piece* piece;
    char* buffer;
};

int GetLineCount(struct chain* chain) {
    return 0;
}

int GetLineLength(struct chain* chain, int line) {
    return 0;
}

int GetLine(struct chain* chain, char* buffer, int* bufferLength, int line) {
    return 0;
}

int Undo(struct chain* chain) {
    return 0;
}

int Redo(struct chain* chain) {
    return 0;
}

int ModifyChain(struct chain* chain, char* text, int lineNumber, int lineOffset, int removeCount) {
    return 0;
}

struct chain InitChain(char* text) {
    struct chain chain;
    chain.buffer = malloc(strlen(text) + 1);
    if (chain.buffer == NULL) exit(1);
    chain.piece = malloc(sizeof(struct piece) * 3);
    if (chain.piece == NULL) exit(1);
    strcpy(chain.buffer, text);
    chain.piece[0].next = 2;
    chain.piece[0].prev = -1;
    chain.piece[0].offset = -1;
    chain.piece[0].length = -1;
    chain.piece[1].next = -1;
    chain.piece[1].prev = 2;
    chain.piece[1].offset = -1;
    chain.piece[1].length = -1;
    // ModifyChain(text, 0, 0, 0);
    chain.piece[2].next = 1;
    chain.piece[2].prev = 0;
    chain.piece[2].offset = 0;
    chain.piece[2].length = strlen(chain.buffer);
    return chain;
}
