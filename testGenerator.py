f = open("opcodeTest.asm", "w")
f.write(".ORIG x3000\n")

arithOpcodes = ["ADD", "AND", "XOR"]
num = "#-1"

f.write(f"RET\n")
f.write(f"RTI\n")
f.write(f"TRAP x25\n")

for arith in arithOpcodes:
    for i in range(8):
         f.write(f"JMP R{i}\n")
         f.write(f"JSRR R{i}\n")
         for j in range(8):
             f.write(f"{arith} R{i} R{j} {num}\n")
             f.write(f"LSHF R{i} R{j} xF\n")
             f.write(f"RSHFL R{i} R{j} xF\n")
             f.write(f"RSHFA R{i} R{j} xF\n")
             f.write(f"NOT R{i} R{j}\n")
             f.write(f"LDB R{i} R{j} {num}\n")
             f.write(f"LDW R{i} R{j} {num}\n")
             f.write(f"STB R{i} R{j} {num}\n")
             f.write(f"STW R{i} R{j} {num}\n")
             for k in range(8):
                 f.write(f"{arith} R{i} R{j} R{k}\n")

f.write(".END\n")
f.close()