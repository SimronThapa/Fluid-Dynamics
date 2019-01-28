
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <stdlib.h>
#include <time.h>
#include <float.h>

#include <curand_kernel.h>

#include "vec3.h"
#include "ray.h"
#include "sphere.h"
#include "triangle.h"
#include "hitable_list.h"
#include "camera.h"
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

// limited version of checkCudaErrors from helper_cuda.h in CUDA examples
#define checkCudaErrors(val) check_cuda( (val), #val, __FILE__, __LINE__ )

using namespace std;
struct VERTEX
{
	float x, y, z;
};
struct FACE
{
	int x, y, z;
};

void ReadOBJFile(const char filename[], float **vertex, int **face, int& VN, int& FN) {
	vector<VERTEX> Vertex;
	vector<FACE> Face;
	std::ifstream file(filename);
	thrust::host_vector<std::string> fileContents;

	//check file existance
	if (!file)
	{
		std::cerr << "Failure opening file at \"" << filename << "\".";
	}

	//file is located
	std::string buffer;
	while (std::getline(file, buffer))
	{
		// Add the buffer contents to our fileContents vector if it's not a comment
		// (Doing the check now reduces memory usage
		if (buffer[0] != '#' || buffer[0] != ' ')
		{
			fileContents.push_back(buffer);
		}
	}
	if (fileContents.size() == 0)
	{
		std::cerr << "File \"" << filename << "\" Was empty... Failure to load\n";
	}
	//save the vertices and faces in the structure
	for (unsigned int n = 0; n < fileContents.size(); n++) {
		//std::cout << fileContents[n].c_str()[0] << "\n";
		
		if (fileContents[n].c_str()[0] == 'v')
		{
			float tmpx, tmpy, tmpz;
			sscanf(fileContents[n].c_str(), "v %f %f %f", &tmpx, &tmpy, &tmpz);
			VERTEX tmpVert = { tmpx, tmpy, tmpz };
			//cout <<"v " <<tmpVert.x << " " << tmpVert.y << " " << tmpVert.z <<"\n";
			Vertex.push_back(tmpVert);
		}
		else if (fileContents[n].c_str()[0] == 'f') {
			int tmpx, tmpy, tmpz;
			sscanf(fileContents[n].c_str(), "f %d %d %d", &tmpx, &tmpy, &tmpz);
			FACE tmpFace = { tmpx, tmpy, tmpz };
			//cout << "f " << tmpFace.x <<" "<< tmpFace.y << " " << tmpFace.z << "\n";
			Face.push_back(tmpFace);
		}
	}

	if ((Vertex.size() != 0) || (Face.size() != 0))
	{
		VN = Vertex.size();
		FN = Face.size();
		cout << "This .obj file has " << Vertex.size() << " vertexs" << endl;
		cout << "This .obj file has " << Face.size() << " faces" << endl;
	}
	for (int i = 0; i < 3; ++i) {
		vertex[i] = new float[Vertex.size()];
		face[i] = new int[Face.size()];
	}
	for (int i = 0; i < Vertex.size(); i++)
	{
		vertex[0][i] = Vertex[i].x;
		vertex[1][i] = Vertex[i].y;
		vertex[2][i] = Vertex[i].z;
	}
	for (int i = 0; i < Face.size(); i++)
	{
		face[0][i] = Face[i].x;
		face[1][i] = Face[i].y;
		face[2][i] = Face[i].z;
	}

}
void check_cuda(cudaError_t result, char const *const func, const char *const file, int const line) {
	if (result) {
		std::cerr << "CUDA error = " << static_cast<unsigned int>(result) << " at " <<
			file << ":" << line << " '" << func << "' \n";
		// Make sure we call CUDA Device Reset before exiting
		cudaDeviceReset();
		exit(99);
	}
}

__device__ vec3 color(const ray& r, hitable **world) {
	hit_record rec;
	if ((*world)->hit(r, 0.0, FLT_MAX, rec)) {
		printf("it hit!\n");
		return 0.5f*vec3(rec.normal.x() + 1.0f, rec.normal.y() + 1.0f, rec.normal.z() + 1.0f);
	}
	else {
		printf("it not hit!\n");
		vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5f*(unit_direction.y() + 1.0f);
		return (1.0f - t)*vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
	}
}

__global__ void render_init(int max_x, int max_y, curandState *rand_state) {
	int i = threadIdx.x + blockIdx.x * blockDim.x;
	int j = threadIdx.y + blockIdx.y * blockDim.y;
	if ((i >= max_x) || (j >= max_y)) return;
	int pixel_index = j * max_x + i;
	//Each thread gets same seed, a different sequence number, no offset
	curand_init(1984, pixel_index, 0, &rand_state[pixel_index]);
}

__global__ void render(vec3 *fb, int max_x, int max_y, int ns, camera **cam, hitable **world /*,curandState *rand_state*/) {
	int i = threadIdx.x + blockIdx.x * blockDim.x;
	int j = threadIdx.y + blockIdx.y * blockDim.y;
	if ((i >= max_x) || (j >= max_y)) return;
	int pixel_index = j * max_x + i;
	/*
	curandState local_rand_state = rand_state[pixel_index];
	vec3 col(0, 0, 0);
	for (int s = 0; s < ns; s++) {
		float u = float(i + curand_uniform(&local_rand_state)) / float(max_x);
		float v = float(j + curand_uniform(&local_rand_state)) / float(max_y);
		ray r = (*cam)->get_ray(u, v);
		col += color(r, world);
	}
	printf("color = %f \n", col / float(ns));
	fb[pixel_index] = col / float(ns);
	*/

	float u = float(i) / float(max_x);
	float v = float(j) / float(max_y);
	ray r = (*cam)->get_ray(u, v);
	fb[pixel_index] = color(r, world);

}

__global__ void create_world(hitable **d_list, hitable **d_world, camera **d_camera, float *vertex,  int *face, int VN, int FN) {
	int index = threadIdx.x;
	int stride = blockDim.x;		
	for (int i = index; i < FN * 3; i+= stride) {
		int idx1 = face[i]; i++;
		int idx2 = face[i]; i++;
		int idx3 = face[i];
		if(i == index)
			*(d_list) = new triangle(vec3(vertex[idx1 * 3], vertex[idx1 * 3 + 1], vertex[idx1 * 3 + 2]), vec3(vertex[idx2 * 3 ], vertex[idx2 * 3 + 1], vertex[idx2 * 3 + 2]), vec3(vertex[idx3 * 3], vertex[idx3 * 3 + 1], vertex[idx3 * 3 + 2]), vec3(0, 0, 1), 1);
		else
			*(d_list + i) = new triangle(vec3(vertex[idx1 * 3], vertex[idx1 * 3 + 1], vertex[idx1 * 3 + 2]), vec3(vertex[idx2 * 3], vertex[idx2 * 3 + 1], vertex[idx2 * 3 + 2]), vec3(vertex[idx3 * 3], vertex[idx3 * 3 + 1], vertex[idx3 * 3 + 2]), vec3(0, 0, 1), 1);
		
		//printf("%f, %f,%f \n", vertex[idx1 * 3], vertex[idx1 * 3 + 1], vertex[idx1 * 3 + 2]);
		//printf("%f, %f,%f \n", vertex[idx2 * 3], vertex[idx2 * 3 + 1], vertex[idx2 * 3 + 2]);
		//printf("%f, %f,%f \n", vertex[idx3 * 3], vertex[idx3 * 3 + 1], vertex[idx3 * 3 + 2]);
	}
	//*(d_list + 1) = new sphere(vec3(0, 0, -1), 0.5);
	*d_world = new hitable_list(d_list, sizeof(face));
	*d_camera = new camera();

}

__global__ void free_world(hitable **d_list, hitable **d_world, camera **d_camera, int FN) {
	delete *(d_list);
	//delete *(d_list + 1);
	for (int i = 1; i < FN; i++) {
		delete *(d_list + i );
	}
	delete *d_world;
	delete *d_camera;
}

int main() {
	int nx = 512;
	int ny = 512;
	int ns = 100;
	int tx = 8;
	int ty = 8;

	std::cerr << "Rendering a " << nx << "x" << ny << " image with " << ns << " samples per pixel ";
	std::cerr << "in " << tx << "x" << ty << " blocks.\n";

	int num_pixels = nx * ny;
	size_t fb_size = num_pixels * sizeof(vec3);

	// allocate FB
	vec3 *fb;
	checkCudaErrors(cudaMallocManaged((void **)&fb, fb_size));

	// allocate random state
	curandState *d_rand_state;
	checkCudaErrors(cudaMalloc((void **)&d_rand_state, num_pixels * sizeof(curandState)));

	// make our world of hitables & the camera
	hitable **d_list;
	checkCudaErrors(cudaMalloc((void **)&d_list, 2 * sizeof(hitable *)));
	hitable **d_world;
	checkCudaErrors(cudaMalloc((void **)&d_world, sizeof(hitable *)));
	camera **d_camera;
	checkCudaErrors(cudaMalloc((void **)&d_camera, sizeof(camera *)));

	// copy the vertex and face from host to device 
	float **VertexK = new float *[3];
	int **FaceK = new int*[3];
	int VN = 0, FN = 0;
	ReadOBJFile("G:\\outputFile\\data_part77.obj", VertexK, FaceK, VN, FN);
	
	// Allocate Unified Memory – accessible from CPU or GPU
	float *Vertex;
	cudaMallocManaged(&Vertex, sizeof(float)* VN * 3);

	int *Face;
	cudaMallocManaged(&Face, sizeof(int)* FN * 3);

	//unified variable initialization
	int counter = 0;
	for (int i = 0; i < FN; i++) {
		Face[counter] = FaceK[0][i]; counter++;
		Face[counter] = FaceK[1][i]; counter++;
		Face[counter] = FaceK[2][i]; counter++;	
	}
	counter = 0;
	for (int i = 0; i < VN; i++) {
		Vertex[counter] = VertexK[0][i]; counter++;
		Vertex[counter] = VertexK[1][i]; counter++;
		Vertex[counter] = VertexK[2][i]; counter++;
	}

	// Run kernel on FN*3 elements on the GPU
	/*
		counter = 0;
		for (int i = 1; i <= VN*3; i++) {
			printf("%f ", Vertex[i-1]);
			if (i % 3 == 0)
				printf("\n");
		}
	*/
	
	create_world << <1, 1 >> > (d_list, d_world, d_camera, Vertex, Face, VN, FN);
	
	// Wait for GPU to finish before accessing on host
	checkCudaErrors(cudaGetLastError());
	checkCudaErrors(cudaDeviceSynchronize());

	clock_t start, stop;
	start = clock();
	// Render our buffer
	dim3 blocks(nx / tx + 1, ny / ty + 1);
	dim3 threads(tx, ty);
	//render_init << <blocks, threads >> > (nx, ny, d_rand_state);
	//checkCudaErrors(cudaGetLastError());
	//checkCudaErrors(cudaDeviceSynchronize());
	render << <blocks, threads >> > (fb, nx, ny, ns, d_camera, d_world);
	checkCudaErrors(cudaGetLastError());
	checkCudaErrors(cudaDeviceSynchronize());
	stop = clock();
	double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
	std::cerr << "took " << timer_seconds << " seconds.\n";

	// Output FB as Image
	std::cout << "P3\n" << nx << " " << ny << "\n255\n";
	FILE *fp = fopen("test.ppm", "wb");
	fprintf(fp, "P6\n%d %d\n255\n", nx, ny);
	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			size_t pixel_index = j * nx + i;
			int ir = int(255.99*fb[pixel_index].r());
			int ig = int(255.99*fb[pixel_index].g());
			int ib = int(255.99*fb[pixel_index].b());
			//std::cout << ir << " " << ig << " " << ib << "\n";
			static unsigned char color[3];
			color[0] = ir;//r
			color[1] = ig;//g
			color[2] = ib;//b
			fwrite(color, 1, 3, fp);
		}
	}

	// clean up
	checkCudaErrors(cudaDeviceSynchronize());
	free_world << <1, 1 >> > (d_list, d_world, d_camera, FN);
	checkCudaErrors(cudaGetLastError());
	checkCudaErrors(cudaFree(d_camera));
	checkCudaErrors(cudaFree(d_world));
	checkCudaErrors(cudaFree(d_list));
	checkCudaErrors(cudaFree(d_rand_state));
	checkCudaErrors(cudaFree(fb));
	checkCudaErrors(cudaFree(Vertex));
	checkCudaErrors(cudaFree(Face));
	

	// useful for cuda-memcheck --leak-check full
	cudaDeviceReset();
}



