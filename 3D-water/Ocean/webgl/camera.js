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

// Constructor for a camera object which manages the view matrix. The Camera
// looks at a point in world space (focus). The position of the camera
// relative to this point is given in spherical coordinates (radius, zenith
// (vertical), and azimuth (horizontal)). Both angles are in radians. The 
// focus is a Vector.
function Camera(focus, radius, zenith, azimuth) {
  this.focus = focus;
  this.radius = radius;
  this.zenith = zenith;
  this.azimuth = azimuth;
}


Math.clamp = function(min, x, max) {
  return Math.min(max, Math.max(x, min));
}


// Moves the camera in or out. The distance is quadratically scaled, which
// has a more natural feel.
Camera.prototype.zoom = function (delta) {
  var MIN_RADIUS = 5.0;
  var MAX_RADIUS = 50.0;
  var r = Math.pow(Math.sqrt(this.radius) - 0.33 * delta, 2);
  this.radius = Math.clamp(MIN_RADIUS, r, MAX_RADIUS);
}


// Rotates the camera around the focus. dx modifies the azimuthal angle, 
// dy modifies the zenith angle.
Camera.prototype.rotate = function (dx, dy) {
  var MIN_ZENITH = 0.1 * Math.PI;
  var MAX_ZENITH = 0.45 * Math.PI
  var z = this.zenith - 0.01 * dy;
  this.zenith = Math.clamp(MIN_ZENITH, z, MAX_ZENITH);
  this.azimuth -= 0.01 * dx;
}


// Calculates the location of the camera in world space. Returns a Vector.
Camera.prototype.position = function () {
  var position = new Vector([
    this.radius * Math.cos(this.azimuth) * Math.sin(this.zenith),
    this.radius * Math.sin(this.azimuth) * Math.sin(this.zenith),
    this.radius * Math.cos(this.zenith)
  ]).add(this.focus);
  return position;
}


// Returns a view matrix for the camera's current configuration.
Camera.prototype.matrix = function () {
  var eye = this.position();
  var f = this.focus.subtract(eye).normal();
  
  var s = new Vector([
    Math.cos(this.azimuth + Math.PI/2.),
    Math.sin(this.azimuth + Math.PI/2.),
    0.0]);

  var u = s.cross(f);

  var look = [
    s[0],  s[1],  s[2], 0.0,
    u[0],  u[1],  u[2], 0.0,
   -f[0], -f[1], -f[2], 0.0,
    0.0,   0.0,    0.0, 1.0
  ];
  return new Matrix(look).translate(-eye[0], -eye[1], -eye[2]);
}
