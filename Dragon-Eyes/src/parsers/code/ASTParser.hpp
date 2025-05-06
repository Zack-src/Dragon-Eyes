
#ifndef ASTPARSER_HPP
#define ASTPARSER_HPP

#include <vector>
#include <string>
#include "../../data_model/DataModel.hpp"
#include <clang-c/Index.h>

namespace DragonEyes {

	class ASTParser
	{
	public:
		ASTParser(const std::vector<std::string>& args);
		~ASTParser();

		void parseFile(SourceFile& f);
	private:
		CXIndex index_;
		std::vector<const char*> clangArgs_;

		static CXChildVisitResult visitor(CXCursor c, CXCursor parent, CXClientData clientData);

		static std::string toString(CXString s);
		static DragonEyes::AccessSpecifier toAccessSpec(CXCursor c);

	};

}

#endif // !ASTPARSER_HPP
