#include "platform/platform.h"

int main()
{

	Platform plt{};
	App::setup(plt);
	plt.loop(App::get());

    return 0;
}
