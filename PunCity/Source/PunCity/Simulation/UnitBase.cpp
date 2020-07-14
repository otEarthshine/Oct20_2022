#include "UnitBase.h"

std::string TransformStateLabel(TransformState state)
{
	static const std::string Label[] =
	{
	   "Stand",
	   "Moving",
	   "NeedTargetAtom",

	   "NeedActionUpdate",
	};

	return Label[static_cast<int>(state)];
}
