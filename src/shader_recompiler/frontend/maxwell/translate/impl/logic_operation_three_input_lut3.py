# Copyright © 2022 degasus <markus@selfnet.de>
# This work is free. You can redistribute it and/or modify it under the
# terms of the Do What The Fuck You Want To Public License, Version 2,
# as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.

# Assume that the negation call is for free on the host GPU.
# In fact, there are often input or output negation flags for logic operations
support_neg = True

# The primitive instructions
OPS = {
    'ir.BitwiseAnd({lhs}, {rhs})' : lambda a,b: a&b,
    'ir.BitwiseOr({lhs}, {rhs})' : lambda a,b: a|b,
    'ir.BitwiseXor({lhs}, {rhs})' : lambda a,b: a^b,
}
if support_neg:
    OPS.update({
        'ir.BitwiseNot(ir.BitwiseAnd({lhs}, {rhs}))' : lambda a,b: 256 + ~(a&b),
        'ir.BitwiseNot(ir.BitwiseOr({lhs}, {rhs}))' : lambda a,b: 256 + ~(a|b),
        'ir.BitwiseNot(ir.BitwiseXor({lhs}, {rhs}))' : lambda a,b: 256 + ~(a^b),
    })

# Our database of combination of instructions
optimized_calls = {}
def register(imm, instruction, count, latency):
    # Use the sum of instruction count and latency as metrik to evaluate which combination is best
    metrik = count + latency

    # Update if new or better
    if imm not in optimized_calls or optimized_calls[imm][3] > metrik:
        optimized_calls[imm] = (instruction, count, latency, metrik)
        return True

    return False

# Constants: 0, 1 (for free)
register(0, 'ir.Imm32(0)', 0, 0)
register(255, 'ir.Imm32(0xFFFFFFFF)', 0, 0)

# Inputs: a, b, c (for free)
ta = 0xF0
tb = 0xCC
tc = 0xAA
inputs = {
    ta : 'a',
    tb : 'b',
    tc : 'c',
}
if support_neg:
    inputs.update({
        256 + ~ta : 'ir.BitwiseNot(a)',
        256 + ~tb : 'ir.BitwiseNot(b)',
        256 + ~tc : 'ir.BitwiseNot(c)',
    })
for imm, instruction in inputs.items():
    register(imm, instruction, 0, 0)

# Try to combine two values from the db with an instruction.
# If it is better than the old method, update it.
while True:
    registered = 0
    calls_copy = optimized_calls.copy()
    for imm_a, (value_a, count_a, latency_a, metrik_a) in calls_copy.items():
        for imm_b, (value_b, count_b, latency_b, metrik_b) in calls_copy.items():
            for OP, f in OPS.items():
                registered += register(
                    f(imm_a, imm_b), # Updated code
                    OP.format(lhs=value_a, rhs=value_b), # New instruction string
                    count_a+count_b+1, # Sum of instructions + 1
                    max(latency_a, latency_b) + 1) # max latency + 1
    if registered == 0:
        # No update at all? So terminate
        break

# Hacky output. Please improve me to output valid C++ instead.
s = """    case {imm}:
        return {op};"""
for imm in range(256):
    print(s.format(imm=imm, op=optimized_calls[imm][0]))