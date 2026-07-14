#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"
#include "tokenizer.h"

int compile(std::string script,
            std::vector<int>& g_bytecode,
            std::vector<std::string>& stringPool,
            int& variableIndex, bool debugInfo = false
);

void compileExpression(std::string expr, 
    std::vector<int>& bytecode, 
    std::unordered_map<std::string, int>& variableMap, 
    int& variableIndex
);
