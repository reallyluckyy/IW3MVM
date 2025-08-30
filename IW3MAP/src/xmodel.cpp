#include "xmodel.hpp"

std::vector<codm_XModel> LoadXModels(GfxWorld& gfxWorld)
{
	std::vector<codm_XModel> xmodels;

	for (int i = 0; i < gfxWorld.modelCount; i++)
	{
		int modelIndex = xmodels.size();

		for (int j = 0; j < xmodels.size(); j++)
		{
			if (std::string(xmodels[j].model->name) == std::string(gfxWorld.models[i].model->name))
			{
				modelIndex = j;
				break;
			}
		}

		if (modelIndex == xmodels.size())
		{
			codm_XModel newXmodel;

			newXmodel.model = gfxWorld.models[i].model;

			xmodels.push_back(newXmodel);
		}

		codm_XModelInstance newInstance;

		newInstance.pos[0] = gfxWorld.models[i].placement.origin[0];
		newInstance.pos[1] = gfxWorld.models[i].placement.origin[1];
		newInstance.pos[2] = gfxWorld.models[i].placement.origin[2];

		newInstance.rot[0][0] = gfxWorld.models[i].placement.rotation[0][0];
		newInstance.rot[0][1] = gfxWorld.models[i].placement.rotation[0][1];
		newInstance.rot[0][2] = gfxWorld.models[i].placement.rotation[0][2];
		newInstance.rot[1][0] = gfxWorld.models[i].placement.rotation[1][0];
		newInstance.rot[1][1] = gfxWorld.models[i].placement.rotation[1][1];
		newInstance.rot[1][2] = gfxWorld.models[i].placement.rotation[1][2];
		newInstance.rot[2][0] = gfxWorld.models[i].placement.rotation[2][0];
		newInstance.rot[2][1] = gfxWorld.models[i].placement.rotation[2][1];
		newInstance.rot[2][2] = gfxWorld.models[i].placement.rotation[2][2];

		newInstance.scale = gfxWorld.models[i].placement.scale;

		xmodels[modelIndex].instances.push_back(newInstance);
	}

	return xmodels;
}