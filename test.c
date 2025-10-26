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
        printf("\t%s [ %spassed%s ]\r\n", text, GREEN, RESET);
    }
    else {
        test.failed++;
        printf("\t%s [ %sfailed%s ]\r\n", text, RED, RESET);
    }
}

void TestResult() {
    printf("\t%sPassed: %s%d%s,  Failed: %s%d%s\r\n", BOLD, GREEN, test.passed, DEFAULT, RED, test.failed, RESET);
}

void TestInitChain() {
    char* test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain = InitChain(test);
    TestEval("TestInitChain buffer", strcmp(chain.buffer, test), 0);
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineCount() {
    char* test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain = InitChain(test);
    int count = GetLineCount(&chain);
    TestEval("testGetLineCount", count, 3);
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineLength() {
    char* test = "wads up doc?\nnot much\nim a wabbit\n";
    struct chain chain = InitChain(test);
    int length;
    length = GetLineLength(&chain, 1);
    TestEval("TestGetLineLength first", length, 12);
    length = GetLineLength(&chain, 2);
    TestEval("TestGetLineLength middle", length, 8);
    length = GetLineLength(&chain, 4);
    TestEval("TestGetLineLength last and only newline", length, 0);
    length = GetLineLength(&chain, 5);
    TestEval("TestGetLineLength after last line", length, -1);
    free(chain.buffer);
    free(chain.piece);
}

int main() {
    TestInitChain();
    TestGetLineCount();
    TestGetLineLength();
    TestResult();
    return 0;
}
