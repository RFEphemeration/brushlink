#ifndef BRUSHLINK_DICTIONARY_H
#define BRUSHLINK_DICTIONARY_H

#include "BuiltinTypedefs.h"

#include "Element.hpp"

namespace Command
{

using Dictionary = Farb::Table<ElementName, Farb::value_ptr<Element> >;

const Dictionary builtins;

} // namespace Command

#endif // BRUSHLINK_DICTIONARY_H