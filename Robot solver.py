from sympy import *

def PrintMatrix(mat):
    for row in mat.tolist():
        print(row)

letters = []
jvecs = []

for x in range(15):
    cha = chr(ord('a') + x)
    globals()[cha] = symbols(cha);
    letters.append(globals()[cha])

for x in range(5):
    cha = "J"+chr(ord('0')+x)
    globals()[cha] = symbols(cha)
    jvecs.append(globals()[cha])

print(letters)

size = 5
c_tr = 2*size+1
c_up = 2*size-1
print("Size =", size)

tr = Matrix([[letters[((c_tr-y)*y)//2] if x == y else 0 for x in range(size)] for y in range(size)])
up = Matrix([[letters[((c_up-y)*y)//2 + x] if y < x else 0 for x in range(size)] for y in range(size)])

jtj = tr + up + up.transpose()

PrintMatrix(jtj)
print("Determinant:")
jtjd = simplify(jtj.det())
print(simplify(jtjd))

print("Inverse, without determinant:")
jtji_d = simplify(jtj.inv()*jtjd)
PrintMatrix(jtji_d)

print("Moore-Penrose Psuedoinverse, without determinant:")
mpi_d = simplify(jtji_d*Matrix([jvecs[x] for x in range(size)]))
PrintMatrix(mpi_d)

