#include "platform/platform.h"

int main()
{

	Platform plt{};
	App app{ plt, Shader{ Shaders::mesh_v, Shaders::mesh_f } };
	plt.loop(app);

    return 0;
}
