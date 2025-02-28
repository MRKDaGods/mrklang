#pragma once

#include "token.h"

MRK_NS_BEGIN

class Lexer;

namespace lexer_position_tree::detail {
	struct Node {
		LexerPosition position;
		Node* parent;

		Node(const LexerPosition& position, Node* const& parent) 
			: position(position), parent(parent) {
		}
	};
}

class LexerPositionTree {
	using Node = lexer_position_tree::detail::Node;
	using PLexer = Lexer*;

public:
	LexerPositionTree(const PLexer lexer);
	~LexerPositionTree();

	const LexerPosition& pushPosition(const LexerPosition& position);
	const LexerPosition& pushPosition();
	void popPosition();
	const LexerPosition& currentPosition();
	const LexerPosition& offsetPosition(uint32_t levels);
	const LexerPosition& parentPosition();

    /**
    * @brief Calculates the difference between the current lexer position and the current tree position
    */
    LexerPosition deltaPosition();

private:
	const PLexer lexer_;
	Node* current_;
};

MRK_NS_END