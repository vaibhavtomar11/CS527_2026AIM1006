// Determinant of 3x3 matrix [a b c; d e f; g h i]
// det = a*(e*i - f*h) - b*(d*i - f*g) + c*(d*h - e*g)
Read x1, 0    // a
Read x2, 1    // b
Read x3, 2    // c
Read x4, 3    // d
Read x5, 4    // e
Read x6, 5    // f
Read x7, 6    // g
Read x8, 7    // h
Read x9, 8    // i

x10 = x5 * x9    // e*i
x11 = x6 * x8    // f*h
x12 = x10 - x11  // e*i - f*h

x13 = x4 * x9    // d*i
x14 = x6 * x7    // f*g
x15 = x13 - x14  // d*i - f*g

x16 = x4 * x8    // d*h
x17 = x5 * x7    // e*g
x18 = x16 - x17  // d*h - e*g

x19 = x1 * x12   // a*(e*i - f*h)
x20 = x2 * x15   // b*(d*i - f*g)
x21 = x3 * x18   // c*(d*h - e*g)

x22 = x19 - x20
x23 = x22 + x21  // determinant

Write x23, 20
