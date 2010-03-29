#ifndef METROBOTICS_PLAYERSRV_COMMAND_H
#define METROBOTICS_PLAYERSRV_COMMAND_H

#include "boost/shared_ptr.hpp"
#include <string>

//* Abstract base class from which all commands are derived.
//* Remarks: Only use this class to create a new class of commands.
//*          Otherwise, use the Command class below when dealing with command objects.
class CommandInterface {
	public:
		// Execute the command.
		virtual void operator()() = 0;

		// Compare commands according to their priority or identification.
		virtual int priority() const = 0;
		virtual std::string id() const = 0;
		friend bool operator<(const CommandInterface& lhs, const CommandInterface& rhs);
		friend bool operator==(const CommandInterface& lhs, const CommandInterface& rhs);
};

//* A handle to any command that is derived from CommandInterface.
//* Remarks: I'm adding this extra layer of abstraction to achieve full class
//*          polymorphism without requiring the user to mess around with pointers
//*          to the base class except when explicitly allocating a new command object.
class Command {
	public:
		Command() {} // Null command.
		Command(CommandInterface* cmd);

		// Returns true if and only if this is not the null command.
		operator bool() const { return mCmd; }

		// Conform to the behaviors specified in CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;
		friend bool operator<(const Command& lhs, const Command& rhs);
		friend bool operator==(const Command& lhs, const Command& rhs);

	private:
		boost::shared_ptr<CommandInterface> mCmd;
};

//* Binary predicate that compares commands according to their priority.
class CommandPriority {
	bool operator()(const CommandInterface& lhs, const CommandInterface& rhs) const;
	bool operator()(const Command& lhs, const Command& rhs) const;
};

//* Binary predicate that compares commands according to their identification.
class CommandIdentification {
	bool operator()(const CommandInterface& lhs, const CommandInterface& rhs) const;
	bool operator()(const Command& lhs, const Command& rhs) const;
};

#endif
