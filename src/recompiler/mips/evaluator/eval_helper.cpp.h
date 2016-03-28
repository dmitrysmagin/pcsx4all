#pragma once

#define eval(insn, ...) eval_##insn(__VA_ARGS__)

