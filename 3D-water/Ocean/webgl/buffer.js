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

// Creates a vertex buffer object (VBO) from a given data array. "data" must
// be a typed array (e.g., Float32Array). "components" is an integer indicating
// how many values are in each element. For example, in a buffer of vectors
// containing XYZ coordinates, "components" would be 3.
function createBuffer(data, components) {
  var count = data.length / components;
  if (count * components != data.length)
    throw "invalid buffer size";

  var type = determineBufferType(data);

  var buffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
  gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);
  buffer.count = count;
  buffer.components = components;
  buffer.type = type;

  return buffer;
}


// Creates an index buffer for use with gl.drawElements. "data" must be a 
// Uint8Array or Uint16Array.
function createIndexBuffer(data) {
  var count = data.length;
  var type = determineBufferType(data);

  var buffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffer);
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, data, gl.STATIC_DRAW);
  buffer.count = count;
  buffer.type = type;
  return buffer;
}


function determineBufferType(data) {
  var constructorTypes = [
    [Uint8Array, gl.UNSIGNED_BYTE],
    [Uint16Array, gl.UNSIGNED_SHORT],
    [Uint32Array, gl.UNSIGNED_INT],
    [Int8Array, gl.BYTE],
    [Int16Array, gl.SHORT],
    [Int32Array, gl.INT],
    [Float32Array, gl.FLOAT],
    [Float64Array, gl.DOUBLE]
  ];
  for (var i = 0; i < constructorTypes.length; i++) {
    if (data instanceof constructorTypes[i][0]) {
      return constructorTypes[i][1];
    }
  }
  throw "invalid buffer type";
}
