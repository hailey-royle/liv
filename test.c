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
    char* test = "wads up doc?\0";
    struct chain chain = InitChain(test);
    if (strcmp(chain.buffer, test)) TestFailed("TestInitChain");
    else TestPassed("TestInitChain");
    free(chain.buffer);
    free(chain.piece);
}

int main() {
    TestInitChain();
    TestResult();
    return 0;
}
