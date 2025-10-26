struct piece {
    int next;
    int prev;
    int offset;
    int length;
};

struct chain {
    struct piece* piece;
    char* buffer;
    int pieceCount;
};

int GetLineCount(struct chain* chain) {
    if (chain == NULL) return -1;
    if (chain->piece == NULL) return -1;
    if (chain->buffer == NULL) return -1;
    int piece = 0;
    int offset = 0;
    int lineCount = 1;
    while (chain->piece[piece].next != -1) {
        if (offset >= chain->piece[piece].length) {
            piece = chain->piece[piece].next;
            offset = 0;
            continue;
        }
        if (chain->buffer[chain->piece[piece].offset + offset] == '\n') {
            lineCount++;
        }
        offset++;
    }
    return lineCount;
}

int GetLineLength(struct chain* chain, int line) {
    if (chain == NULL) return -1;
    if (chain->piece == NULL) return -1;
    if (chain->buffer == NULL) return -1;
    if (line < 1) return -1;
    int piece = 0;
    int offset = 0;
    int length = 0;
    while (line > 1) {
        if (chain->piece[piece].next == -1) return -1;
        if (offset >= chain->piece[piece].length) {
            piece = chain->piece[piece].next;
            offset = 0;
            continue;
        }
        if (chain->buffer[chain->piece[piece].offset + offset] == '\n') {
            line--;
        }
        offset++;
    }
    while (true) {
        if (chain->piece[piece].next == -1) break;
        if (offset >= chain->piece[piece].length) {
            piece = chain->piece[piece].next;
            offset = 0;
            continue;
        }
        if (chain->buffer[chain->piece[piece].offset + offset] == '\n') {
            break;
        }
        offset++;
        length++;
    }
    return length;
}

int GetLine(struct chain* chain, char* buffer, int bufferLength, int line) {
    if (chain == NULL) return -1;
    if (chain->piece == NULL) return -1;
    if (chain->buffer == NULL) return -1;
    if (line < 1) return -1;
    int piece = 0;
    int offset = 0;
    while (line > 1) {
        if (chain->piece[piece].next == -1) return -1;
        if (offset >= chain->piece[piece].length) {
            piece = chain->piece[piece].next;
            offset = 0;
            continue;
        }
        if (chain->buffer[chain->piece[piece].offset + offset] == '\n') {
            line--;
        }
        offset++;
    }
    while (bufferLength > 1) {
        if (chain->piece[piece].next == -1) break;
        if (offset >= chain->piece[piece].length) {
            piece = chain->piece[piece].next;
            offset = 0;
            continue;
        }
        if (chain->buffer[chain->piece[piece].offset + offset] == '\n') {
            break;
        }
        *buffer = chain->buffer[chain->piece[piece].offset + offset];
        buffer++;
        bufferLength--;
        offset++;
    }
    *buffer = '\0';
    return 0;
}

int Undo(struct chain* chain) {
    return 0;
}

int Redo(struct chain* chain) {
    return 0;
}

int ModifyChain(struct chain* chain, char* text, int lineNumber, int lineOffset, int removeCount) {
    if (chain == NULL) return -1;
    if (chain->piece == NULL) return -1;
    if (chain->buffer == NULL) return -1;
    if (text == NULL) return -1;
    if (lineNumber < 1) return -1;
    if (lineOffset < 0) return -1;
    if (removeCount < 0) return -1;
    // todo: find where the text needs to be modifyed, dont just assume the very beginning
    int piece = 2;
    int offset = 0;
    if (removeCount > 0) {

        while (removeCount > chain->piece[piece].length) {
            chain->piece[piece].length = 0;
            piece = chain->piece[piece].next;
        }
        chain->piece[piece].offset += removeCount;
        chain->piece[piece].length -= removeCount;
    }
    if (strlen(text) > 0) {
        struct piece* newPiece = realloc(chain->piece, sizeof(chain->piece[0]) * (chain->pieceCount + 2));
        if (newPiece == NULL) return -1;
        chain->piece = newPiece;
        chain->piece[chain->pieceCount].next = chain->pieceCount + 1;
        chain->piece[chain->pieceCount].prev = piece;
        chain->piece[chain->pieceCount].offset = strlen(chain->buffer);
        chain->piece[chain->pieceCount].length = strlen(text);
        chain->piece[chain->pieceCount + 1].next = chain->piece[piece].next;
        chain->piece[chain->pieceCount + 1].prev = chain->pieceCount;
        chain->piece[chain->pieceCount + 1].offset = chain->piece[piece].offset + offset;
        chain->piece[chain->pieceCount + 1].length = chain->piece[piece].length - offset;
        chain->piece[chain->piece[piece].next].prev = chain->pieceCount + 1;
        chain->piece[piece].next = chain->pieceCount;
        chain->piece[piece].length = offset;
        chain->pieceCount += 2;
        char* newBuffer = realloc(chain->buffer, strlen(text) + strlen(chain->buffer) + 1);
        if (newBuffer == NULL) return -1;
        chain->buffer = newBuffer;
        strcat(chain->buffer, text);
        return 1;
    }

    return 0;
}

struct chain InitChain(char* text) {
    struct chain chain;
    chain.buffer = malloc(strlen(text) + 1);
    chain.piece = malloc(sizeof(struct piece) * 3);
    if (chain.buffer == NULL) exit(1);
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
    chain.piece[2].next = 1;
    chain.piece[2].prev = 0;
    chain.piece[2].offset = 0;
    chain.piece[2].length = strlen(chain.buffer);
    chain.pieceCount = 3;
    return chain;
}
