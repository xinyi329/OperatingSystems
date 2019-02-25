Lab 1: Linker

Great Job!!

RES 33 of 34:  1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0

LABRESULTS = 40 + 58.2300 - 0 = 98.2300

----------------------------------------------------------------------------------------------

Comments:

Input-34 is an off-by-one error => When a module have only 4 instructions, "relative 4" has already exceed the module's size (valid offsets are 0, 1, 2, 3)
----------------------------------------------------------------------------------------------
################### input-34##################
###    Warning Diff:    .....
###    Code/Error Diff: .....
15c15
< 006: 1004 Error: Relative address exceeds module size; zero used
---
> 006: 1008