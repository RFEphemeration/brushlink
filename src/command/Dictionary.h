#ifndef BRUSHLINK_DICTIONARY_H
#define BRUSHLINK_DICTIONARY_H

namespace Command
{

using Dictionary = Table<ElementName, value_ptr<Element> >;

const Dictionary builtins;

} // namespace Command

#endif // BRUSHLINK_DICTIONARY_H