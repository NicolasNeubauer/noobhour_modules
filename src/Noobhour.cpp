#include "Noobhour.hpp"


Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;

	p->addModel(modelBaseliner);
	p->addModel(modelBsl1r);	
	p->addModel(modelCustomscaler);	
}
