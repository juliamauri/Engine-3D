#ifndef __RE_SKYBOX_H__
#define __RE_SKYBOX_H__

#include "Resource.h"
struct RE_SkyBoxSettings {
};
class RE_SkyBox :
	public ResourceContainer
{
public:
	RE_SkyBox();
	~RE_SkyBox();
};

#endif // !__RE_SKYBOX_H__