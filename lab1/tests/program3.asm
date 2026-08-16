// Multiply two complex numbers (a+bi) * (c+di)
// real = a*c - b*d , imag = a*d + b*c
Read x1, 0    // a
Read x2, 1    // b
Read x3, 2    // c
Read x4, 3    // d
x5 = x1 * x3   // a*c
x6 = x2 * x4   // b*d
x7 = x5 - x6   // real part
x8 = x1 * x4   // a*d
x9 = x2 * x3   // b*c
x10 = x8 + x9  // imag part
Write x7, 10
Write x10, 11
