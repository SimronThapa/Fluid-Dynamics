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

// Constructor for a very minimal 3-vector type.
function Vector(elements) {
  if (elements.length != 3)
    throw "invalid vector length";
  this[0] = elements[0];
  this[1] = elements[1];
  this[2] = elements[2];
}


Vector.prototype.toString = function () {
  return "<" + this[0] + "," + this[1] + "," + this[2] + ">";
}


Vector.prototype.add = function (v) {
  var elems = [
    this[0] + v[0],
    this[1] + v[1],
    this[2] + v[2]
  ];
  return new Vector(elems);
}


Vector.prototype.subtract = function (v) {
  var elems = [
    this[0] - v[0],
    this[1] - v[1],
    this[2] - v[2]
  ];
  return new Vector(elems);
}


Vector.prototype.negate = function () {
  var elems = [
    -this[0], 
    -this[1],
    -this[2]
  ];
  return new Vector(elems);
}


Vector.prototype.length = function () {
  return Math.sqrt(this[0] * this[0] + this[1] * this[1] + this[2] * this[2]);
}


Vector.prototype.normal = function () {
  var len = this.length();
  var elems = [
    this[0] / len,
    this[1] / len,
    this[2] / len
  ];
  return new Vector(elems);
}


Vector.prototype.scale = function (s) {
  var elems = [
    this[0] * s,
    this[1] * s, 
    this[2] * s
  ];
  return new Vector(elems);
}


Vector.prototype.dot = function (v) {
  return this[0] * v[0] + this[1] * v[1] + this[2] * v[2];
}


Vector.prototype.cross = function (v) {
  var elems = [
    this[1] * v[2] - this[2] * v[1],
    this[2] * v[0] - this[0] * v[2],
    this[0] * v[1] - this[1] * v[0]
  ];
  return new Vector(elems);
}
