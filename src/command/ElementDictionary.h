#ifndef BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H
#define BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H

#include "ElementTypes.hpp"

#include <set>

using namespace Farb;

namespace Command
{

struct NextTokenCriteria;

class ElementDictionary
{
	static const std::map<ElementName, ElementDeclaration> declarations;

public:
	static const ElementDeclaration * GetDeclaration(ElementName name);

	static void GetAllowedNextElements(const NextTokenCriteria & criteria, std::set<ElementName> & out_allowed);
};

} // namespace Command

#endif // BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H