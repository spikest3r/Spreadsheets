#pragma once
#include "includes.h"
#include "helpers.h"
#include "types.h"

using NativeFn = std::function<void(std::vector<Variant>&, std::vector<Variant>&)>;

int64_t getInt(const Variant& v);

extern std::unordered_map<int, NativeFn> funcMap;

extern std::thread vmThread;

int run(
    std::unique_ptr<VMProgramData> progData
);

int execute(
    VMProgramData* progData,
    VMExecutionData* execData
);

void setVMhalt();
bool vmRunning();
