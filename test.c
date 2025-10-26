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

void TestFailed(char* text) {
    test.failed++;
    printf("\t%s [ %sfailed%s ]\r\n", text, RED, RESET);
}

void TestPassed(char* text) {
    test.passed++;
    printf("\t%s [ %spassed%s ]\r\n", text, GREEN, RESET);
}

void TestResult() {
    printf("\t%sPassed: %s%d%s,  Failed: %s%d%s\r\n", BOLD, GREEN, test.passed, DEFAULT, RED, test.failed, RESET);
}

void TestInitChain() {
    char* test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain = InitChain(test);
    if (!strcmp(chain.buffer, test)) TestPassed("TestInitChain buffer");
    else TestFailed("TestInitChain buffer");
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineCount() {
    char* test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain = InitChain(test);
    int count = GetLineCount(&chain);
    if (count == 3) TestPassed("TestGetLineCount");
    else TestFailed("TestGetLineCount");
    free(chain.buffer);
    free(chain.piece);
}

void TestGetLineLength() {
    char* test = "wads up doc?\nnot much\nim a wabbit";
    struct chain chain = InitChain(test);
    int length = GetLineLength(&chain, 2);
    if (length == 8) TestPassed("TestGetLineLength");
    else TestFailed("TestGetLineLength");
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
