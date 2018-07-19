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

// Mesh objects manage all the data needed to draw something with WebGL.
// - shader (see shader.js): an object containing a compiled and linked 
//     shader object. Uniform and attribute locations will be stored here.
// - uniforms: a dictionary of uniform values to pass to the shader. Each key
//     is the name of a uniform. Each value is an object with properties:
//     - fn: a function to upload the uniform (e.g., gl.uniform1f)
//     - value: the value to upload.
// - attributes: a dictionary of vertex attributes. Each key is the name of
//     an attribute. Each value is a VBO (see buffer.js).
// - textures: a dictionary of texture objects. Each key is the name of a
//     sampler2D uniform. Each value is a texture object.
// - primitive: the primitive to be drawn, e.g., gl.TRIANGLES.
function Mesh(shader, uniforms, attributes, textures, primitive) {
  this.shader = shader;
  this.uniforms = uniforms;
  this.attributes = attributes;
  this.textures = textures;
  this.primitive = primitive;

  for (var name in uniforms) {
    this.shader.findUniform(name);
  }

  var count;
  for (var name in attributes) {
    this.shader.findAttribute(name);

    var buffer = attributes[name];
    if (count === void 0) {
      count = buffer.count;
    } else if (buffer.count != count) {
      throw "attribute buffers are different sizes!";
    }
  }

  for (var name in textures) {
    this.shader.findUniform(name);
  }

  this.vertexCount = count;
}


// A variant of Mesh intended for use with gl.drawElements. It contains an 
// additional property, "indices", an index buffer.
function IndexedMesh(indices, shader, uniforms, attributes, textures, primitive) {
  Mesh.call(this, shader, uniforms, attributes, textures, primitive);
  this.indices = indices;
}
IndexedMesh.prototype.__proto__ = Mesh.prototype;


// Prepares the mesh for drawing. The shader program is bound, all uniforms,
// attributes, and textures are bound to the appropriate locations.
Mesh.prototype.prepare = function () {
  gl.useProgram(this.shader);

  for (var name in this.uniforms) {
    var fn = this.uniforms[name].fn;
    var value = this.uniforms[name].value;
    fn.call(gl, this.shader[name], value);
  }

  for (var name in this.attributes) {
    gl.enableVertexAttribArray(this.shader[name]);
    gl.bindBuffer(gl.ARRAY_BUFFER, this.attributes[name]);
    gl.vertexAttribPointer(this.shader[name],
                           this.attributes[name].components,
                           this.attributes[name].type,
                           false, 0, 0);
  }

  var textureUnit = 0;
  for (var name in this.textures) {
    gl.activeTexture(gl.TEXTURE0 + textureUnit);
    gl.bindTexture(gl.TEXTURE_2D, this.textures[name]);
    gl.uniform1i(this.shader[name], textureUnit);
    textureUnit++;
  }
}


// Draws a Mesh using gl.drawArrays. prepare should be called first.
Mesh.prototype.draw = function () {
  gl.drawArrays(this.primitive, 0, this.vertexCount);
}


// Set a value in the uniforms dictionary. Does not update WebGL.
Mesh.prototype.setUniform = function (name, value) {
  if (!(name in this.uniforms))
    throw "invalid uniform name";
  this.uniforms[name].value = value;
}


// Prepares an IndexedMesh for drawing. This calls Mesh.prepare, and it 
// also binds the index buffer.
IndexedMesh.prototype.prepare = function () {
  Mesh.prototype.prepare.call(this);
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.indices);
}


// Draws an IndexedMesh using gl.drawElements. prepare should be called first.
IndexedMesh.prototype.draw = function () {
  gl.drawElements(this.primitive, this.indices.count, this.indices.type, 0);
}
