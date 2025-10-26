#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "piecechain.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define DEFAULT "\x1b[39m"
#define BOLD    "\x1b[1m"
#define RESET   "\x1b[0m"

struct test {
    int passed;
    int failed;
};

struct test test;

void TestEval(char* text, int one, int two) {
    if (one == two) {
        test.passed++;
        printf("\t[ %spassed%s ] %s\r\n", GREEN, RESET, text);
    }
    else {
        test.failed++;
        printf("\t[ %sfailed%s ] %s\r\n", RED, RESET, text);
    }
}

void TestResult() {
    printf("\t%sPassed: %s%d%s,  Failed: %s%d%s\r\n", BOLD, GREEN, test.passed, DEFAULT, RED, test.failed, RESET);
}

void TestInitChain() {
    char* test;
    test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain;
    chain = InitChain(test);
    TestEval("InitChain buffer", strcmp(chain.buffer, test), 0);
    test = "";
    free(chain.buffer);
    free(chain.piece);
    chain = InitChain(test);
    TestEval("InitChain buffer empty input", strcmp(chain.buffer, test), 0);
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineCount() {
    char* test = "wads up doc?\nnot much\nim a wabbit\n";
    struct chain chain = InitChain(test);
    int count = GetLineCount(&chain);
    TestEval("GetLineCount", count, 4);
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineLength() {
    char* test = "wads up doc?\nnot much\nim a wabbit\n";
    struct chain chain = InitChain(test);
    int length;
    length = GetLineLength(&chain, 1);
    TestEval("GetLineLength first", length, 12);
    length = GetLineLength(&chain, 2);
    TestEval("GetLineLength middle", length, 8);
    length = GetLineLength(&chain, 4);
    TestEval("GetLineLength last and only newline", length, 0);
    length = GetLineLength(&chain, 5);
    TestEval("GetLineLength after last line", length, -1);
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLine() {
    char* test = "wads up doc?\nnot much\nim a wabbit\n";
    struct chain chain = InitChain(test);
    int lineLength = 32;
    char line1[lineLength];
    char line2[lineLength];
    char line3[lineLength];
    char line4[lineLength];
    GetLine(&chain, line1, lineLength, 1);
    TestEval("GetLine first", strcmp(line1, "wads up doc?"), 0);
    GetLine(&chain, line2, lineLength, 2);
    TestEval("GetLine middle", strcmp(line2, "not much"), 0);
    GetLine(&chain, line3, lineLength, 4);
    TestEval("GetLine last and only newline", strcmp(line3, ""), 0);
    int res = GetLine(&chain, line4, lineLength, 5);
    TestEval("GetLine after last line", res, -1);
    free(chain.buffer);
    free(chain.piece);
}

int main() {
    TestInitChain();
    TestGetLineCount();
    TestGetLineLength();
    TestGetLine();
    TestResult();
    return 0;
}
