#include "Noobhour.hpp"


Plugin *plugin;


void init(Plugin *p) {
	plugin = p;

	p->addModel(modelBaseliner);
	p->addModel(modelBsl1r);	
	p->addModel(modelCustomscaler);	
}
