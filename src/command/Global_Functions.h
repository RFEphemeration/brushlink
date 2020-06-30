#ifndef BRUSHLINK_GLOBAL_FUNCTIONS_H
#define BRUSHLINK_GLOBAL_FUNCTIONS_H

namespace Command
{

struct NumberFromDigits
{
	static ErrorOr<Number> Evaluate(Context & context, std::vector<Digit> digits);
	static std::string Print(const GlobalFunction & element, std::string line_prefix);
}


} // namespace Command

#endif // BRUSHLINK_GLOBAL_FUNCTIONS_H
