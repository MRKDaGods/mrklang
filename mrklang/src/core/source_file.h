#pragma once

#include "common/types.h"

#include <sstream>

MRK_NS_BEGIN

struct SourceFile {
	Str filename;

	// Contents are relatively small so we can store them directly
	// 06/03/2025
	struct {
		Str raw;

		Vec<Str> lines() const {
			Vec<Str> lines;
			std::istringstream sourceStream(raw);
			Str line;
			while (std::getline(sourceStream, line)) {
				lines.push_back(line);
			}

			return lines;
		}
	} contents;
};

MRK_NS_END