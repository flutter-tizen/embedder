// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ax_platform_text_boundary.h"

#include "ax/ax_enums.h"

namespace ui {

#ifdef OS_LINUX
ax::mojom::TextBoundary FromAtkTextBoundary(AtkTextBoundary boundary) {
  // These are listed in order of their definition in the ATK header.
  switch (boundary) {
    case ATK_TEXT_BOUNDARY_CHAR:
      return ax::mojom::TextBoundary::kCharacter;
    case ATK_TEXT_BOUNDARY_WORD_START:
      return ax::mojom::TextBoundary::kWordStart;
    case ATK_TEXT_BOUNDARY_WORD_END:
      return ax::mojom::TextBoundary::kWordEnd;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      return ax::mojom::TextBoundary::kSentenceStart;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      return ax::mojom::TextBoundary::kSentenceEnd;
    case ATK_TEXT_BOUNDARY_LINE_START:
      return ax::mojom::TextBoundary::kLineStart;
    case ATK_TEXT_BOUNDARY_LINE_END:
      return ax::mojom::TextBoundary::kLineEnd;
  }
}

#if ATK_CHECK_VERSION(2, 10, 0)
ax::mojom::TextBoundary FromAtkTextGranularity(AtkTextGranularity granularity) {
  // These are listed in order of their definition in the ATK header.
  switch (granularity) {
    case ATK_TEXT_GRANULARITY_CHAR:
      return ax::mojom::TextBoundary::kCharacter;
    case ATK_TEXT_GRANULARITY_WORD:
      return ax::mojom::TextBoundary::kWordStart;
    case ATK_TEXT_GRANULARITY_SENTENCE:
      return ax::mojom::TextBoundary::kSentenceStart;
    case ATK_TEXT_GRANULARITY_LINE:
      return ax::mojom::TextBoundary::kLineStart;
    case ATK_TEXT_GRANULARITY_PARAGRAPH:
      return ax::mojom::TextBoundary::kParagraphStart;
  }
}
#endif  // ATK_CHECK_VERSION(2, 10, 0)
#endif  // OS_LINUX

}  // namespace ui
