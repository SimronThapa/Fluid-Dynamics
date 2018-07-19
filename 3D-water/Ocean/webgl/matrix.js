// Copyright (c) 2012, Jay Conrod
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * The name of Jay Conrod may not be used to endorse or promote products
//       derived from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL JAY CONROD BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Constructor for a minimal 4x4 matrix type intended for graphics operations.
// The constructor expects elements in row-major order, but they are stored 
// in a Float32Array in column major order. Instances of this constructor 
// should be considered immutable.
function Matrix(elements) {
  if (elements !== void 0 && elements.length != 16)
    throw "invalid elements length";
  this.elements = new Float32Array(16);
  if (elements) {
    for (var r = 0; r < 4; r++) {
      for (var c = 0; c < 4; c++) {
        this.elements[c*4 + r] = elements[r*4 + c];
      }
    }
  }
}


var identityMatrix = new Matrix([
  1.0, 0.0, 0.0, 0.0,
  0.0, 1.0, 0.0, 0.0,
  0.0, 0.0, 1.0, 0.0,
  0.0, 0.0, 0.0, 1.0
]);


Matrix.prototype.get = function (row, column) {
  return this.elements[column * 4 + row];
}


Matrix.prototype.set = function (row, column, value) {
  this.elements[column * 4 + row] = value;
}


Matrix.prototype.multiply = function (right) {
  var left = this;
  var result = new Matrix();
  for (var i = 0; i < 4; i++) {
    for (var j = 0; j < 4; j++) {
      var v = 0.0;
      for (var k = 0; k < 4; k++) {
        v += left.get(i, k) * right.get(k, j);
      }
      result.set(i, j, v);
    }
  }
  return result;
}
  

// Based on glTranslate
Matrix.prototype.translate = function(x, y, z) {
  var translation = new Matrix([
    1.0, 0.0, 0.0, x,
    0.0, 1.0, 0.0, y,
    0.0, 0.0, 1.0, z,
    0.0, 0.0, 0.0, 1.0
  ]);
  return this.multiply(translation);
}


// Based on glRotate
Matrix.prototype.rotate = function(angle, x, y, z) {
  var c = Math.cos(angle);
  var s = Math.sin(angle);
  var rotation = new Matrix([
    x*x*(1-c)+c,   x*y*(1-c)-z*s, x*z*(1-c)+y*s, 0.0,
    y*x*(1-c)+z*s, y*y*(1-c)+c,   y*z*(1-c)-x*s, 0.0,
    x*z*(1-c)-y*s, y*z*(1-c)+x*s, z*z*(1-c)+c,   0.0,
    0.0,           0.0,           0.0,           1.0
  ]);
  return this.multiply(rotation);
}


// Based on glScale
Matrix.prototype.scale = function(x, y, z) {
  var scale = new Matrix([
      x, 0.0, 0.0, 0.0,
    0.0,   y, 0.0, 0.0,
    0.0, 0.0,   z, 0.0,
    0.0, 0.0, 0.0, 1.0
  ]);
  return this.multiply(scale);
}


// Based on gluPerspective
Matrix.prototype.perspective = function(fovy, aspect, zNear, zFar) {
  var f = 1.0 / Math.tan(fovy / 2.0);
  var perspective = new Matrix([
    f/aspect, 0.0, 0.0, 0.0,
    0.0, f, 0.0, 0.0,
    0.0, 0.0, (zNear+zFar)/(zNear-zFar), (2.*zFar*zNear)/(zNear-zFar),
    0.0, 0.0, -1.0, 0.0
  ]);
  return this.multiply(perspective);
}


// Based on gluLookat
Matrix.prototype.lookAt = function(eyeX, eyeY, eyeZ,
                                   centerX, centerY, centerZ,
                                   upX, upY, upZ) {
  var f = [centerX - eyeX, centerY - eyeY, centerZ - eyeZ];
  var fLength = Math.sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
  var fNorm = [f[0]/fLength, f[1]/fLength, f[2]/fLength];

  var upLength = Math.sqrt(upX * upX + upY * upY + upZ * upZ);
  var upNorm = [upX/upLength, upY/upLength, upZ/upLength];

  var s = [
    fNorm[1] * upNorm[2] - fNorm[2] * upNorm[1],
    fNorm[2] * upNorm[0] - fNorm[0] * upNorm[2],
    fNorm[0] * upNorm[1] - fNorm[1] * upNorm[0]
  ];
  var u = [
    s[1] * fNorm[2] - s[2] * fNorm[1],
    s[2] * fNorm[0] - s[0] * fNorm[2],
    s[0] * fNorm[1] - s[1] * fNorm[0]
  ];

  var lookAt = new Matrix([
    s[0],  s[1],  s[2], 0.0,
    u[0],  u[1],  u[2], 0.0,
   -fNorm[0], -fNorm[1], -fNorm[2], 0.0,
     0.0,   0.0,   0.0, 1.0
  ]);
  return this.multiply(lookAt).translate(-eyeX, -eyeY, -eyeZ);
}
