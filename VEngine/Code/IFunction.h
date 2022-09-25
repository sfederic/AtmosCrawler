#pragma once
#include <vector>
#include <typeindex>
#include <string>

//Base class for VFunctions
class IFunction
{
protected:
	std::string name;

	std::vector<std::type_index> argTypes;
	std::vector<std::string> argNames;

public:
	IFunction(std::string name_);

	auto GetArgTypes() { return argTypes; }
	std::string GetName(int index) { return argNames[index]; }
};
