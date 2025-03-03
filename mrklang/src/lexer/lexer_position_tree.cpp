#include "lexer_position_tree.h"
#include "lexer.h"
#include "common/logging.h"

MRK_NS_BEGIN

LexerPositionTree::LexerPositionTree(const PLexer lexer) : lexer_(lexer), current_(nullptr) {
}

LexerPositionTree::~LexerPositionTree() {
	while (current_) {
		popPosition();
	}
}

const LexerPosition& LexerPositionTree::pushPosition(const LexerPosition& position) {
	current_ = new Node(position, current_);
	return position;
}

const LexerPosition& LexerPositionTree::pushPosition() {
	return pushPosition(lexer_->getPosition());
}

void LexerPositionTree::popPosition() {
	if (!current_) {
		MRK_WARN("Attempted to end position with no active node");
		return;
	}

	auto* parent = current_->parent;
	delete current_;
	current_ = parent;
}

const LexerPosition& LexerPositionTree::currentPosition() {
	if (!current_) {
		MRK_WARN("Attempted to read current position with no active node");
		return {};
	}

	return current_->position;
}

const LexerPosition& LexerPositionTree::offsetPosition(uint32_t levels) {
	if (!current_) {
		MRK_WARN("Attempted to offset position with no active node");
		return {};
	}

	auto cur = current_;
	for (int i = 0; i < levels && cur; i++) {
		cur = cur->parent;
	}

	if (!cur) {
		MRK_WARN("Attempted to offset position at an invalid node, current={} levels={}", current_->position.toString(), levels);
		return {};
	}

	return cur->position;
}

const LexerPosition& LexerPositionTree::parentPosition() {
	return offsetPosition(1);
}

LexerPosition LexerPositionTree::deltaPosition() {
	return std::remove_cvref_t<decltype(lexer_->getPosition())>(lexer_->getPosition()) - currentPosition();
}

MRK_NS_END