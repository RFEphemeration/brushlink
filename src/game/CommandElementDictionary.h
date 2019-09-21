#ifndef BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H
#define BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H

#include "CommandElementTypes.hpp"

using namespace Farb;

namespace Command
{

using ElementVariant = std::variant<
	ElementAtomDynamic,
	ElementLiteral,
	ElementReference,
	ElementWord,
	ElementAtom< > > // we need to cover every kind of atom type here, don't we?

class ElementDictionary : Singleton<ElementDictionary>
{
	std::unordered_map<ElementID, CRef<Element> > elements;

	const Element & Get(ElementID id)
	{
		elements[id];
	}

	std::unordered_map<ElementID, ElementAtomDynamic> element_atoms_dynamic;
	std::unordered_map<ElementID, ElementLiteral> element_literals;
	// should only contain "@-1, @0, @1, etc"
	std::unordered_map<ElementID, ElementReference> element_references;
	std::unordered_map<ElementID, ElementReference> element_words;

private:
	void InitializeTypedAtoms();



}

} // namespace Command

#endif // BRUSHFIRE_COMMAND_ELEMENT_DICTIONARY_H