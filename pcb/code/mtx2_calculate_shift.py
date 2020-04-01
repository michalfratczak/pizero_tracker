#!/bin/env python
from __future__ import print_function

'''
Calculate frequency shift for MTX2 module.
Reference to resistor numbers in Eagle project.
'''

R1 = 4700
R2 = 4700
R3 = 22000 + 1000 + 470 # R3 + R4 + R5 in Eagle

Vcc = 3.3 # volts

R1_R3 = 1.0/(1.0/R1 + 1.0/R3)
R2_R3 = 1.0/(1.0/R2 + 1.0/R3)

V_Low  = R2_R3 / (R2_R3+R1) * Vcc
V_High = R2 / (R2+R1_R3) * Vcc
V_Diff = V_High - V_Low
FreqShift = V_Diff * 2000

print('R1', R1)
print('R2', R2)
print('R3', R3)
print('R1|R3', R1_R3)
print('R2|R3', R2_R3)
print('Vcc', Vcc)
print('V_Low', V_Low)
print('V_High', V_High)
print('V_Diff', V_Diff)
print('FreqShift', FreqShift)
