#include "Command.h"
using namespace std;

Command::Command(CommandInterface* cmd)
:mCmd(cmd)
{
}

void Command::operator()()
{
	// Make sure the pointer is valid.
	CommandInterface* ptr = mCmd.get();
	if (ptr == 0) {
		return;
	} else {
		// Execute the command.
		(*ptr)();
	}
}

int Command::priority() const
{
	// Make sure the pointer is valid.
	CommandInterface* ptr = mCmd.get();
	if (ptr == 0) {
		return -1;
	} else {
		return ptr->priority();
	}
}

string Command::id() const
{
	// Make sure the pointer is valid.
	CommandInterface* ptr = mCmd.get();
	if (ptr == 0) {
		return string();
	} else {
		return ptr->id();
	}
}

bool operator<(const CommandInterface& lhs, const CommandInterface& rhs)
{
	return lhs.priority() < rhs.priority();
}

bool operator==(const CommandInterface& lhs, const CommandInterface& rhs)
{
	return lhs.id() == rhs.id();
}

bool operator<(const Command& lhs, const Command& rhs)
{
	return lhs.priority() < rhs.priority();
}

bool operator==(const Command& lhs, const Command& rhs)
{
	return lhs.id() == rhs.id();
}

bool CommandPriority::operator()(const CommandInterface& lhs, const CommandInterface& rhs) const
{
	return lhs < rhs;
}

bool CommandPriority::operator()(const Command& lhs, const Command& rhs) const
{
	return lhs < rhs;
}

bool CommandIdentification::operator()(const CommandInterface& lhs, const CommandInterface& rhs) const
{
	return lhs == rhs;
}

bool CommandIdentification::operator()(const Command& lhs, const Command& rhs) const
{
	return lhs == rhs;
}
