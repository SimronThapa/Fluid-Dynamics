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

// Given GLSL source code for a vertex and fragment shader, this function 
// compiles and links a shader program, reporting any errors. The methods
// findAttribute and findUniform are added for convenience.
function createShaderFromSource(vertexSource, fragmentSource) {
  var vertexShader = compileShader(vertexSource, gl.VERTEX_SHADER);
  var fragmentShader = compileShader(fragmentSource, gl.FRAGMENT_SHADER);
  var program = gl.createProgram();
  gl.attachShader(program, vertexShader);
  gl.attachShader(program, fragmentShader);
  gl.linkProgram(program);
  var result = gl.getProgramParameter(program, gl.LINK_STATUS);
  if (!result)
    throw "error linking shader!";

  program.findAttribute = findShaderAttribute;
  program.findUniform = findShaderUniform;

  return program;
}


// Extracts shader source code from the document (given the ids of elements
// containing the source), compiles, links, and returns a shader program.
// This is the easiest way to build a shader.
function createShaderFromIds(vertexId, fragmentId) {
  var vertexSource = getShaderSourceFromId(vertexId);
  var fragmentSource = getShaderSourceFromId(fragmentId);
  var shader = createShaderFromSource(vertexSource, fragmentSource);
  return shader;
}


// Extracts the source for a shader from the document, given the id of the
// element containing it.
function getShaderSourceFromId(id) {
  var shaderScript = document.getElementById(id);
  if (!shaderScript)
    throw "could not locate script: " + id;

  var source = "";
  var currentChild = shaderScript.firstChild;
  while (currentChild) {
    if (currentChild.nodeType == 3)
      source += currentChild.textContent;
    currentChild = currentChild.nextSibling;
  }
  if (source.length === 0)
    throw "shader is empty: " + id;

  return source;
}


// Finds the location of a vertex attribute. The location is memoized as a 
// property of the shader object.
function findShaderAttribute(name) {
  gl.useProgram(this);
  if (!(name in this)) {
    this[name] = gl.getAttribLocation(this, name);
  }
  return this[name];    
}


// Finds the location of a uniform. The location is memoized as a property 
// of the shader object.
function findShaderUniform(name) {
  gl.useProgram(this);
  if (!(name in this)) {
    this[name] = gl.getUniformLocation(this, name);
  }
  return this[name];
}


// Compiles a shader. If there are any errors, the message is thrown as an
// exception.
function compileShader(source, type) {
  var shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  var result = gl.getShaderParameter(shader, gl.COMPILE_STATUS);
  if (!result) {
    var error = gl.getShaderInfoLog(shader);
    throw error;
  }
  return shader;
}
