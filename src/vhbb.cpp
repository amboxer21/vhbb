#include "vhbb.h"

#include "network.h"
#include "database.h"
#include "activity.h"
#include "Views/CategoryView/categoryView.h"
#include "Views/background.h"
#include "Views/statusBar.h"
#include "input.h"
#include "splash.h"


// Davee: Fix c++ exceptions
extern "C"
{
    unsigned int sleep(unsigned int seconds)
    {
        sceKernelDelayThread(seconds*1000*1000);
        return 0;
    }

    int usleep(useconds_t usec)
    {
        sceKernelDelayThread(usec);
        return 0;
    }

    void __sinit(struct _reent *);
}

__attribute__((constructor(101)))
void pthread_setup(void)
{
    pthread_init();
    __sinit(_REENT);
}


int main()
{
	sceIoMkdir(VHBB_DATA.c_str(), 0777);

	vita2d_init();
	vita2d_set_clear_color(COLOR_BLACK);

	displaySplash();

	dbg_init();

	#ifdef PSP2SHELL
	// Do it before any file is opened otherwise psp2shell fails to reload the app
	sceAppMgrUmount("app0:");
	psp2shell_init(3333, 0);
	dbg_printf(DBG_INFO, "PSPSHELL started on port 3333");
	#endif

	Network &network = *Network::create_instance();

	try {
		// TODO check if fails
		network.Download(API_ENDPOINT, API_LOCAL);
		auto db = Database::create_instance(API_LOCAL);
		dbg_printf(DBG_DEBUG, "Instance created");
		db->DownloadIcons();
	} catch (const std::exception &ex) {
		dbg_printf(DBG_ERROR, "Couldn't load database: %s", ex.what());
		throw ex;
	}

	Input input;

	Activity &activity = *Activity::create_instance();

	Background background;

	StatusBar statusBar;
	CategoryView categoryView;

	// Init queue view too

	GUIViews curView = LIST_VIEW;
	while (1) {
		//sceKernelPowerTick(0);
		vita2d_start_drawing();
		vita2d_clear_screen();

		input.Get();
		//input.Propagate(curView); // TODO: Rework function
		if (!activity.HasActivity()) {
			categoryView.HandleInput(1, input);
		} else {
			categoryView.HandleInput(0, input);
		}

		activity.HandleInput(1, input);

		background.Display();
		if (!activity.HasActivity())
			categoryView.Display();

		statusBar.Display();

		activity.Display();

		vita2d_end_drawing();
		vita2d_swap_buffers();
		sceDisplayWaitVblankStart();
	}

	#ifdef PSP2SHELL
	psp2shell_exit();
	#endif

	return 0;
}

