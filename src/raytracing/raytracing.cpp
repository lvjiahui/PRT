#include "raytracing.h"
#include "platform/platform.h"
#include <iostream>
#include <fmt/core.h>
#include <execution>
#include <cmath>
#include <algorithm>
#include <random>

const float PI = 3.14159265359;

inline float random() {
	static std::uniform_real_distribution<float> distribution(0.0, 1.0);
	static std::mt19937 generator;
	return distribution(generator);
}

/*
 * We will register this error handler with the device in initializeDevice(),
 * so that we are automatically informed on errors.
 * This is extremely helpful for finding bugs in your code, prevents you
 * from having to add explicit error checking to each Embree API call.
 */
void errorFunction(void* userPtr, enum RTCError error, const char* str)
{
	printf("error %d: %s\n", error, str);
}

/*
 * Embree has a notion of devices, which are entities that can run
 * raytracing kernels.
 * We initialize our device here, and then register the error handler so that
 * we don't miss any errors.
 *
 * rtcNewDevice() takes a configuration string as an argument. See the API docs
 * for more information.
 *
 * Note that RTCDevice is reference-counted.
 */
RTCDevice initializeDevice()
{
	RTCDevice device = rtcNewDevice(NULL);

	if (!device)
		printf("error %d: cannot create device\n", rtcGetDeviceError(NULL));

	rtcSetDeviceErrorFunction(device, errorFunction, NULL);
	return device;
}
RTCDevice device = initializeDevice();


struct Vertex { float x, y, z; };
struct Triangle { int v0, v1, v2; };

RTScene::RTScene(Mesh& gl_mesh)
{
	scene = rtcNewScene(device);
	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
	int num_vertices = gl_mesh.verts().size();
	int num_faces = gl_mesh.indices().size() / 3;
	Vertex* vertices = (Vertex*)rtcSetNewGeometryBuffer(geom,
		RTC_BUFFER_TYPE_VERTEX,
		0,
		RTC_FORMAT_FLOAT3,
		sizeof(Vertex),
		num_vertices);
	Triangle* indices = (Triangle*)rtcSetNewGeometryBuffer(geom,
		RTC_BUFFER_TYPE_INDEX,
		0,
		RTC_FORMAT_UINT3,
		sizeof(Triangle),
		num_faces);
	if (!vertices || !indices) {
		fmt::print(stderr, "can not set GeometryBuffer\n");
		return;
	}
	for (int i = 0; i != num_vertices; i++) {
		vertices[i].x = gl_mesh.verts()[i].pos.x;
		vertices[i].y = gl_mesh.verts()[i].pos.y;
		vertices[i].z = gl_mesh.verts()[i].pos.z;
	}
	for (int i = 0; i != num_faces; i++) {
		indices[i].v0 = gl_mesh.indices()[3 * i + 0];
		indices[i].v1 = gl_mesh.indices()[3 * i + 1];
		indices[i].v2 = gl_mesh.indices()[3 * i + 2];
	}
	rtcCommitGeometry(geom);
	rtcAttachGeometry(scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(scene);
}

RTScene::~RTScene()
{
	rtcReleaseScene(scene);
}


void init_Ray(RTCRayHit& rayhit,
	const glm::vec3& org,
	const glm::vec3& dir,
	float tnear = 0,
	float tfar = std::numeric_limits<float>::infinity()
)
{
	rayhit.ray.org_x = org.x;
	rayhit.ray.org_y = org.y;
	rayhit.ray.org_z = org.z;
	rayhit.ray.dir_x = dir.x;
	rayhit.ray.dir_y = dir.y;
	rayhit.ray.dir_z = dir.z;
	rayhit.ray.tnear = tnear;
	rayhit.ray.tfar = tfar;
	rayhit.ray.mask = -1;
	rayhit.ray.flags = 0;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
}

inline glm::vec2 UniformSampleDisk(const float u, const float v) {
    float r = std::sqrt(u);
    float theta = 2 * PI * v;
    return glm::vec2(r * std::cos(theta), r * std::sin(theta));
}

/// cosine-weighted sampling of hemisphere oriented along the +z-axis
////////////////////////////////////////////////////////////////////////////////
inline float cosineSampleHemispherePDF(const glm::vec3& dir)
{
	return dir.z / PI;
}
inline glm::vec3 cosineSampleHemisphere(const float u, const float v)
{
    auto d = UniformSampleDisk(u, v);
    float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
    return glm::vec3(d.x, d.y, z);
}
/*! Cosine weighted hemisphere sampling. Up direction is provided as argument. */
using Sample = std::pair<glm::vec3,float>;
inline Sample cosineSampleHemisphere(const float u, const float v, const glm::vec3& N)
{
	glm::vec3 localDir = cosineSampleHemisphere(u, v);
	auto pdf = cosineSampleHemispherePDF(localDir);
	// tangent space calculation from origin point
	glm::vec3 up = abs(N.z) < 0.99 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
	glm::vec3 right = glm::normalize(glm::cross(up, N));
	up = glm::cross(N, right);
	auto dir = localDir.x * up + localDir.y * right + localDir.z * N;
	return {dir, pdf};
}


glm::vec3 renderNormal(RTCScene scene, glm::vec3 pos, glm::vec3 dir) {
  	struct RTCRayHit rayhit;
	  init_Ray(rayhit, pos, dir);
		struct RTCIntersectContext context;
		rtcInitIntersectContext(&context);
		rtcIntersect1(scene, &context, &rayhit);
    if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
			return {};
		}
		glm::vec3 normal{ rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z };
		normal = glm::normalize(normal);
    return 0.5f * (normal + glm::vec3{1});
}

glm::vec3 renderAO(RTCScene scene, glm::vec3 pos, glm::vec3 dir) {
	auto& app = App::get();
	float eps = 1e-5;
	/* radiance accumulator and weight */
	glm::vec3 L(0.0f);
	glm::vec3 Lw(1.0f);
	glm::vec3 Le(1.0f);
	/*
	 * The ray hit structure holds both the ray and the hit.
	 * The user must initialize it properly -- see API documentation
	 * for rtcIntersect1() for details.
	 */
	struct RTCRayHit rayhit;
	init_Ray(rayhit, pos, dir);
	for (int i = 0; i < app.max_path_length; i++) {
		/* terminate if contribution too low */
		if (std::max({ Lw.x,Lw.y,Lw.z }) < 0.01f)
			break;

		struct RTCIntersectContext context;
		rtcInitIntersectContext(&context);
		rtcIntersect1(scene, &context, &rayhit);

		/* invoke environment lights if nothing hit */
		if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
			L = Lw * Le;
			break;
		}

		glm::vec3 normal{ rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z };
		normal = glm::normalize(normal);
		pos += rayhit.ray.tfar * dir;
		auto [wi, pdf] = cosineSampleHemisphere(random(), random(), normal);
		if (pdf <= 1E-4f /* 0.0f */) break;
		// Lw *= glm::clamp(glm::dot(wi, normal), 0.f, 1.f) / (pdf * PI);
		Lw *= glm::vec3{ app.albedo[0], app.albedo[1], app.albedo[2] };
		/* setup secondary ray */
		float sign = glm::dot(wi, normal) < 0.0f ? -1.0f : 1.0f;
		pos += sign * eps * normal;
		init_Ray(rayhit, pos, wi, eps);
	}
	return L;
}

void raytrace(const RTScene& rtscene) {
  auto &app = App::get();
  if (app.camera.dirty){
	  app.pixels_w.assign(app.pixels_w.size(), App::Pixel_accum{});
    app.camera.dirty = false;
  }
	int w = app.plt.SCR_WIDTH, h = app.plt.SCR_HEIGHT;
	assert(app.pixels.size() == w * h);
	assert(app.pixels_w.size() == w * h);
	std::vector<int> lines;
	for (int t = 0; t < h; t++) lines.push_back(t);
	std::for_each(std::execution::par, lines.begin(), lines.end(),
		[&](int j) {

		auto height = 2 * tan(glm::radians(app.camera.Zoom) / 2);
		auto width = height * w / h;
		auto Up = (float)height * app.camera.Up;
		auto Right = (float)width * app.camera.Right;
		auto buttom_left = app.camera.Front - 0.5f * Up - 0.5f * Right;
		for (int i = 0; i != w; i++) {
			auto dir = buttom_left + (float)j / h * Up + (float)i / w * Right;
			auto L = renderAO(rtscene.scene, app.camera.Position, dir);
			// auto L = renderNormal(rtscene.scene, app.camera.Position, dir);
			auto& pixel = app.pixels[j * w + i];
			auto& accum = app.pixels_w[j * w + i];
			accum.color += L;
			accum.count += 1;
			auto weight = 1 / accum.count;
			auto color = glm::clamp(accum.color * weight, glm::vec3(0.f), glm::vec3(1.f));
      if (app.gamma)
        color = glm::pow(color, glm::vec3{1/2.2});
			pixel.r = 255 * color.x;
			pixel.g = 255 * color.y;
			pixel.b = 255 * color.z;
			pixel.a = 255;

		}
	}
	);

}