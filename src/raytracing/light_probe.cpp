
#include "light_probe.h"
#include <execution>
#include <mutex>

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

RTScene::RTScene(Model &gl_model)
{
	scene = rtcNewScene(device);
	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

	size_t num_vertices = 0;
	size_t num_faces = 0;
	for (auto& gl_mesh : gl_model.meshes){
		num_vertices += gl_mesh.verts().size();
		num_faces += gl_mesh.indices().size() / 3;
	}

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
	for (auto& gl_mesh : gl_model.meshes){
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

struct Ray
{
	Ray(
		const glm::vec3& org,
		const glm::vec3& dir,
		float tnear = 0,
		float tfar = std::numeric_limits<float>::infinity()
	){
		rayhit.ray.org_x = org.x;
		rayhit.ray.org_y = org.y;
		rayhit.ray.org_z = org.z;
		rayhit.ray.dir_x = dir.x;
		rayhit.ray.dir_y = dir.y;
		rayhit.ray.dir_z = dir.z;
		rayhit.ray.tnear = tnear;
		rayhit.ray.tfar = tfar;
		rayhit.ray.mask = static_cast<unsigned int>(-1);
		rayhit.ray.flags = 0;
		rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
		rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
	}
	bool first_hit(RTScene& scene) {
		struct RTCIntersectContext context;
		rtcInitIntersectContext(&context); //per intersection
		rtcIntersect1(scene.scene, &context, &rayhit);
		return rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID;
	}
	glm::vec3 hit_normal(){
		return glm::vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
	}
	bool any_hit(RTScene& scene) {
		struct RTCIntersectContext context;
		rtcInitIntersectContext(&context); //per intersection
		rtcOccluded1(scene.scene, &context, &rayhit.ray);
		return !(rayhit.ray.tfar > 0);
	}

	RTCRayHit rayhit;
};


//uniform ray direction
std::vector<glm::vec3> get_dirs(int samples = 100) {
	std::vector<glm::vec3> dirs;
	double pi = 2 * acos(0.0);
	//std::cout << pi << std::endl;
	double gold = 3 - sqrt(5);
	//std::cout << gold << std::endl;
	for (int i = 0; i < samples; i++) {
		double z = 1 - (double(i) / double(samples - 1)) * 2;
		double theta = pi * i * gold;
		double x = cos(theta) * sqrt(1 - z * z);
		double y = sin(theta) * sqrt(1 - z * z);
		//std::cout << x <<" "<< y <<" "<< z << std::endl;
		dirs.push_back(glm::vec3({ x, y, z }));
	}
	return dirs;
}



Volume_weight calculate_weight(Model &gl_model, glm::ivec3 probe_res, glm::ivec3 volume_res, glm::vec3 scene_size){

	RTScene rtscene{gl_model};

	std::vector<int> z_index;
	for (int z_i = 0; z_i < volume_res.z; z_i++)
		z_index.push_back(z_i);

	auto volume_index = [&](int x_i, int y_i, int z_i){
		if(
			x_i < 0|| 
			y_i < 0||
			z_i < 0||
			x_i >= volume_res.x|| 
			y_i >= volume_res.y||
			z_i >= volume_res.z
			)
		return volume_res.x * volume_res.y * volume_res.z;
		return (z_i * volume_res.y + y_i) * volume_res.x + x_i;
	};
	auto cal_probe_pos = [&](glm::ivec3 probe_ID){
		glm::vec3 probe_ds = 2.f / glm::vec3(probe_res) * scene_size;
		glm::vec3 pos = -scene_size + probe_ds * (glm::vec3(0.5) + glm::vec3(probe_ID));
		return pos;
	};
	auto cal_volume_pos = [&](glm::ivec3 volume_ID){
		glm::vec3 volume_ds = 2.f / glm::vec3(volume_res) * scene_size;
		glm::vec3 pos = -scene_size + volume_ds * (glm::vec3(0.5) + glm::vec3(volume_ID));
		return pos;
	};

	static std::vector<glm::vec3> out_dirs = get_dirs();
	// auto try_move_outside = [&](glm::vec3 pos){
	// 	glm::vec3 volume_ds = 2.f / glm::vec3(volume_res) * scene_size;
	// 	float max_move_dist = 1.7 * std::max({volume_ds.x, volume_ds.y, volume_ds.z});
	// 	float min_dist = max_move_dist;
	// 	glm::vec3 new_pos = pos;
	// 	for (auto& dir : out_dirs){
	// 		Ray ray(pos, dir, 0, min_dist);
	// 		if (ray.first_hit(rtscene)){
	// 			if (glm::dot(dir, ray.hit_normal()) > 0.1 && // inside
	// 				ray.rayhit.ray.tfar < min_dist)
	// 			{
	// 				min_dist = ray.rayhit.ray.tfar;
	// 				new_pos = pos + (min_dist + 0.01f) * dir;
	// 			}
	// 		}
	// 	}
	// 	return new_pos;
	// };

	std::vector<float> volume_inside_score(volume_res.x * volume_res.y * volume_res.z);
	std::for_each(std::execution::par, z_index.begin(), z_index.end(),
		[&](int z_i) {
		for (int y_i = 0; y_i < volume_res.y; y_i++)
			for (int x_i = 0; x_i < volume_res.x; x_i++)
			{
				int index = volume_index(x_i, y_i, z_i);
				glm::vec3 pos = cal_volume_pos(glm::ivec3(x_i, y_i, z_i));
				int hit_count = 0;
				volume_inside_score[index] = 0;
				for (auto& dir : out_dirs){
					Ray ray(pos, dir);
					if (ray.first_hit(rtscene)){
						hit_count++;
						if (glm::dot(dir, ray.hit_normal()) > 0.01) {// inside
							volume_inside_score[index]++;
						}
					}
				}
				volume_inside_score[index] /= hit_count;
			}
		});
	volume_inside_score.push_back(999); //the last one for out of index volume;

	auto cal_visible = [&](glm::vec3 volume_pos, glm::ivec3 probe_ID){
		assert(
			probe_ID.x >= -1|| 
			probe_ID.y >= -1||
			probe_ID.z >= -1||
			probe_ID.x <= probe_res.x|| 
			probe_ID.y <= probe_res.y||
			probe_ID.z <= probe_res.z
			);
		if(
			probe_ID.x < 0|| 
			probe_ID.y < 0||
			probe_ID.z < 0||
			probe_ID.x >= probe_res.x|| 
			probe_ID.y >= probe_res.y||
			probe_ID.z >= probe_res.z
			)
			return false;
		glm::vec3 probe_pos{cal_probe_pos(probe_ID)};
		Ray ray(volume_pos, probe_pos - volume_pos, 0, 1);
		if (ray.any_hit(rtscene)) 
			return false;
		return true;
	};

	Volume_weight result;
	result.weight0123.resize(volume_res.x * volume_res.y * volume_res.z);
	result.weight4567.resize(volume_res.x * volume_res.y * volume_res.z);


	std::for_each(std::execution::par, z_index.begin(), z_index.end(),
		[&](int z_i) {
		for (int y_i = 0; y_i < volume_res.y; y_i++)
			for (int x_i = 0; x_i < volume_res.x; x_i++)
			{
				int index = volume_index(x_i, y_i, z_i);
    			glm::vec3 text_coord = (glm::vec3{x_i, y_i, z_i} + glm::vec3{0.5}) / glm::vec3(volume_res); //[0.5/volume_res, 1-0.5/volume_res]
    			text_coord = text_coord * glm::vec3(probe_res) - glm::vec3(0.5); //[-0.5, probe_res-0.5]
				/*
				*       4----7
				*      /|   /|
				*     5----6 |
				*     | 3--|-2
				*     |/   |/
				*     0----1
				*
				*     ^ y
				*     |
				*     |
				*     o---->x
				*    /
				*   /z
				*  v
				*/
    			const glm::ivec3 anchor = glm::ivec3(glm::floor(text_coord));
				glm::ivec3 probe_ID[8];
    			probe_ID[0] = anchor + glm::ivec3(0,0,1);
    			probe_ID[1] = anchor + glm::ivec3(1,0,1);
    			probe_ID[2] = anchor + glm::ivec3(1,0,0);
    			probe_ID[3] = anchor + glm::ivec3(0,0,0);
    			probe_ID[4] = anchor + glm::ivec3(0,1,0);
    			probe_ID[5] = anchor + glm::ivec3(0,1,1);
    			probe_ID[6] = anchor + glm::ivec3(1,1,1);
    			probe_ID[7] = anchor + glm::ivec3(1,1,0);

				glm::vec3 fract = cal_volume_pos(glm::ivec3(x_i, y_i, z_i)) - cal_probe_pos(anchor);
				fract = fract * glm::vec3(probe_res) / (2.f*scene_size);
				assert(fract.x >= 0 &&
					fract.y >= 0 &&
					fract.z >= 0 &&
					fract.x <= 1 &&
					fract.y <= 1 &&
					fract.z <= 1
				);
				std::vector<float> weight{
					(1-fract.x)	*(1-fract.y)*(fract.z)	,
					(fract.x)	*(1-fract.y)*(fract.z)	,
					(fract.x)	*(1-fract.y)*(1-fract.z),
					(1-fract.x)	*(1-fract.y)*(1-fract.z),
					(1-fract.x)	*(fract.y)	*(1-fract.z),
					(1-fract.x)	*(fract.y)	*(fract.z)	,
					(fract.x)	*(fract.y)	*(fract.z)	,
					(fract.x)	*(fract.y)	*(1-fract.z)
				};
				float sum = std::accumulate(weight.begin(), weight.end(), 0.f);
				assert(std::abs(sum - 1) < 0.001);

				//try move to outside before calculate visibility
				glm::vec3 volume_pos = cal_volume_pos(glm::ivec3(x_i, y_i, z_i));
				if (volume_inside_score[index] > 0.2) {
					float min_score = volume_inside_score[index];
					for (int nx = -1; nx <=1; nx++)
					for (int ny = -1; ny <=1; ny++)
					for (int nz = -1; nz <=1; nz++)
					{
						float neigbor_score = volume_inside_score[volume_index(x_i+nx, y_i+ny, z_i+nz)];
						if (neigbor_score < min_score) {
							volume_pos = cal_volume_pos(glm::ivec3(x_i+nx, y_i+ny, z_i+nz));
							min_score = neigbor_score;
						}
					}
				}

				float visibility[8];
				float valid = 0;
				for (int i = 0; i < 8; i++){
					visibility[i] = cal_visible(volume_pos, probe_ID[i]);
					valid += visibility[i];
				}

				if (valid > 0){
					for (int i = 0; i < 8; i++){
						weight[i] *= visibility[i];
					}
					sum = std::accumulate(weight.begin(), weight.end(), 0.f);
					for (int i = 0; i < 8; i++){
						weight[i] /= sum;
					}
				} else { 
					auto min = std::min_element(weight.begin(), weight.end());
					float tmp = *min;
					for (int i = 0; i < 8; i++){
						weight[i] *= 0;
					}
					//hack, keep the smallest weight
					// *min = tmp;
				}

				result.weight0123[index] = glm::vec4{weight[0], weight[1], weight[2], weight[3]};
				result.weight4567[index] = glm::vec4{weight[4], weight[5], weight[6], weight[7]};
			}
	}
	);


	return result;
}
