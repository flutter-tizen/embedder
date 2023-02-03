// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "string_utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <codecvt>
#include <locale>
#include <regex>
#include <sstream>

#include "flutter/fml/string_conversion.h"
#include "third_party/dart/runtime/third_party/double-conversion/src/double-conversion.h"

#include "base/logging.h"
#include "icu_utf.h"
#include "no_destructor.h"

namespace base {

using double_conversion::DoubleToStringConverter;
using double_conversion::StringBuilder;

namespace {
constexpr char kExponentChar = 'e';
constexpr char kInfinitySymbol[] = "Infinity";
constexpr char kNaNSymbol[] = "NaN";

// The number of digits after the decimal we allow before switching to
// exponential representation.
constexpr int kDecimalInShortestLow = -6;
// The number of digits before the decimal we allow before switching to
// exponential representation.
constexpr int kDecimalInShortestHigh = 12;
constexpr int kConversionFlags =
    DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;

const DoubleToStringConverter& GetDoubleToStringConverter() {
  static DoubleToStringConverter converter(
      kConversionFlags, kInfinitySymbol, kNaNSymbol, kExponentChar,
      kDecimalInShortestLow, kDecimalInShortestHigh, 0, 0);
  return converter;
}

std::string NumberToStringImpl(double number, bool is_single_precision) {
  if (number == 0.0) {
    return "0";
  }

  constexpr int kBufferSize = 128;
  std::array<char, kBufferSize> char_buffer;
  StringBuilder builder(char_buffer.data(), char_buffer.size());
  if (is_single_precision) {
    GetDoubleToStringConverter().ToShortestSingle(static_cast<float>(number),
                                                  &builder);
  } else {
    GetDoubleToStringConverter().ToShortest(number, &builder);
  }
  return std::string(char_buffer.data(), builder.position());
}
}  // namespace

std::u16string ASCIIToUTF16(std::string src) {
  return std::u16string(src.begin(), src.end());
}

std::u16string UTF8ToUTF16(std::string src) {
  return fml::Utf8ToUtf16(src);
}

std::string UTF16ToUTF8(std::u16string src) {
  return fml::Utf16ToUtf8(src);
}

std::u16string WideToUTF16(const std::wstring& src) {
  return std::u16string(src.begin(), src.end());
}

std::wstring UTF16ToWide(const std::u16string& src) {
  return std::wstring(src.begin(), src.end());
}

std::u16string NumberToString16(float number) {
  return ASCIIToUTF16(NumberToString(number));
}

std::u16string NumberToString16(int32_t number) {
  return ASCIIToUTF16(NumberToString(number));
}

std::u16string NumberToString16(unsigned int number) {
  return ASCIIToUTF16(NumberToString(number));
}

std::u16string NumberToString16(double number) {
  return ASCIIToUTF16(NumberToString(number));
}

std::string NumberToString(int32_t number) {
  return std::to_string(number);
}

std::string NumberToString(unsigned int number) {
  return std::to_string(number);
}

std::string NumberToString(float number) {
  return NumberToStringImpl(number, true);
}

std::string NumberToString(double number) {
  return NumberToStringImpl(number, false);
}

std::string NumberToString(int64_t number) {
  return std::to_string(number);
}

std::string JoinString(std::vector<std::string> tokens, std::string delimiter) {
  std::ostringstream imploded;
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i == tokens.size() - 1) {
      imploded << tokens[i];
    } else {
      imploded << tokens[i] << delimiter;
    }
  }
  return imploded.str();
}

std::u16string JoinString(std::vector<std::u16string> tokens,
                          std::u16string delimiter) {
  std::u16string result;
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i == tokens.size() - 1) {
      result.append(tokens[i]);
    } else {
      result.append(tokens[i]);
      result.append(delimiter);
    }
  }
  return result;
}

void ReplaceChars(std::string in,
                  std::string from,
                  std::string to,
                  std::string* out) {
  size_t pos = in.find(from);
  while (pos != std::string::npos) {
    in.replace(pos, from.size(), to);
    pos = in.find(from, pos + to.size());
  }
  *out = in;
}

void ReplaceChars(std::u16string in,
                  std::u16string from,
                  std::u16string to,
                  std::u16string* out) {
  size_t pos = in.find(from);
  while (pos != std::u16string::npos) {
    in.replace(pos, from.size(), to);
    pos = in.find(from, pos + to.size());
  }
  *out = in;
}

const std::string& EmptyString() {
  static const base::NoDestructor<std::string> s;
  return *s;
}

std::string ToUpperASCII(std::string str) {
  std::string ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++)
    ret.push_back(std::toupper(str[i]));
  return ret;
}

std::string ToLowerASCII(std::string str) {
  std::string ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++)
    ret.push_back(std::tolower(str[i]));
  return ret;
}

bool LowerCaseEqualsASCII(std::string a, std::string b) {
  std::string lower_a = ToLowerASCII(a);
  return lower_a.compare(ToLowerASCII(b)) == 0;
}

bool ContainsOnlyChars(std::u16string str, char16_t ch) {
  return std::find_if(str.begin(), str.end(),
                      [ch](char16_t c) { return c != ch; }) == str.end();
}

bool StartsWith(std::string str,
                std::string search_for,
                CompareCase case_sensitivity) {
  if (search_for.size() > str.size())
    return false;

  std::string source = str.substr(0, search_for.size());

  switch (case_sensitivity) {
    case CompareCase::SENSITIVE:
      return source == search_for;

    case CompareCase::INSENSITIVE_ASCII:
      return ToLowerASCII(source) == ToLowerASCII(search_for);

    default:
      return false;
  }
}

bool EndsWith(std::string str,
              std::string search_for,
              CompareCase case_sensitivity) {
  if (search_for.size() > str.size())
    return false;

  std::string source =
      str.substr(str.size() - search_for.size(), search_for.size());

  switch (case_sensitivity) {
    case CompareCase::SENSITIVE:
      return source == search_for;

    case CompareCase::INSENSITIVE_ASCII:
      return ToLowerASCII(source) == ToLowerASCII(search_for);

    default:
      return false;
  }
}

bool ReadUnicodeCharacter(const char* src,
                          int32_t src_len,
                          int32_t* char_index,
                          uint32_t* code_point_out) {
  // U8_NEXT expects to be able to use -1 to signal an error, so we must
  // use a signed type for code_point.  But this function returns false
  // on error anyway, so code_point_out is unsigned.
  int32_t code_point;
  CBU8_NEXT(src, *char_index, src_len, code_point);
  *code_point_out = static_cast<uint32_t>(code_point);

  // The ICU macro above moves to the next char, we want to point to the last
  // char consumed.
  (*char_index)--;

  // Validate the decoded value.
  return IsValidCodepoint(code_point);
}

bool ReadUnicodeCharacter(const char16_t* src,
                          int32_t src_len,
                          int32_t* char_index,
                          uint32_t* code_point) {
  if (CBU16_IS_SURROGATE(src[*char_index])) {
    if (!CBU16_IS_SURROGATE_LEAD(src[*char_index]) ||
        *char_index + 1 >= src_len || !CBU16_IS_TRAIL(src[*char_index + 1])) {
      // Invalid surrogate pair.
      return false;
    }

    // Valid surrogate pair.
    *code_point =
        CBU16_GET_SUPPLEMENTARY(src[*char_index], src[*char_index + 1]);
    (*char_index)++;
  } else {
    // Not a surrogate, just one 16-bit word.
    *code_point = src[*char_index];
  }

  return IsValidCodepoint(*code_point);
}

OffsetAdjuster::Adjustment::Adjustment(size_t original_offset,
                                       size_t original_length,
                                       size_t output_length)
    : original_offset(original_offset),
      original_length(original_length),
      output_length(output_length) {}

// static
void OffsetAdjuster::AdjustOffsets(const Adjustments& adjustments,
                                   std::vector<size_t>* offsets_for_adjustment,
                                   size_t limit) {
  BASE_DCHECK(offsets_for_adjustment);
  for (auto& i : *offsets_for_adjustment)
    AdjustOffset(adjustments, &i, limit);
}

// static
void OffsetAdjuster::AdjustOffset(const Adjustments& adjustments,
                                  size_t* offset,
                                  size_t limit) {
  BASE_DCHECK(offset);
  if (*offset == std::u16string::npos)
    return;
  int adjustment = 0;
  for (const auto& i : adjustments) {
    if (*offset <= i.original_offset)
      break;
    if (*offset < (i.original_offset + i.original_length)) {
      *offset = std::u16string::npos;
      return;
    }
    adjustment += static_cast<int>(i.original_length - i.output_length);
  }
  *offset -= adjustment;

  if (*offset > limit)
    *offset = std::u16string::npos;
}

// static
void OffsetAdjuster::UnadjustOffsets(
    const Adjustments& adjustments,
    std::vector<size_t>* offsets_for_unadjustment) {
  if (!offsets_for_unadjustment || adjustments.empty())
    return;
  for (auto& i : *offsets_for_unadjustment)
    UnadjustOffset(adjustments, &i);
}

// static
void OffsetAdjuster::UnadjustOffset(const Adjustments& adjustments,
                                    size_t* offset) {
  if (*offset == std::u16string::npos)
    return;
  int adjustment = 0;
  for (const auto& i : adjustments) {
    if (*offset + adjustment <= i.original_offset)
      break;
    adjustment += static_cast<int>(i.original_length - i.output_length);
    if ((*offset + adjustment) < (i.original_offset + i.original_length)) {
      *offset = std::u16string::npos;
      return;
    }
  }
  *offset += adjustment;
}

// static
void OffsetAdjuster::MergeSequentialAdjustments(
    const Adjustments& first_adjustments,
    Adjustments* adjustments_on_adjusted_string) {
  auto adjusted_iter = adjustments_on_adjusted_string->begin();
  auto first_iter = first_adjustments.begin();
  // Simultaneously iterate over all |adjustments_on_adjusted_string| and
  // |first_adjustments|, pushing adjustments at the end of
  // |adjustments_builder| as we go.  |shift| keeps track of the current number
  // of characters collapsed by |first_adjustments| up to this point.
  // |currently_collapsing| keeps track of the number of characters collapsed by
  // |first_adjustments| into the current |adjusted_iter|'s length.  These are
  // characters that will change |shift| as soon as we're done processing the
  // current |adjusted_iter|; they are not yet reflected in |shift|.
  size_t shift = 0;
  size_t currently_collapsing = 0;
  // While we *could* update |adjustments_on_adjusted_string| in place by
  // inserting new adjustments into the middle, we would be repeatedly calling
  // |std::vector::insert|. That would cost O(n) time per insert, relative to
  // distance from end of the string.  By instead allocating
  // |adjustments_builder| and calling |std::vector::push_back|, we only pay
  // amortized constant time per push. We are trading space for time.
  Adjustments adjustments_builder;
  while (adjusted_iter != adjustments_on_adjusted_string->end()) {
    if ((first_iter == first_adjustments.end()) ||
        ((adjusted_iter->original_offset + shift +
          adjusted_iter->original_length) <= first_iter->original_offset)) {
      // Entire |adjusted_iter| (accounting for its shift and including its
      // whole original length) comes before |first_iter|.
      //
      // Correct the offset at |adjusted_iter| and move onto the next
      // adjustment that needs revising.
      adjusted_iter->original_offset += shift;
      shift += currently_collapsing;
      currently_collapsing = 0;
      adjustments_builder.push_back(*adjusted_iter);
      ++adjusted_iter;
    } else if ((adjusted_iter->original_offset + shift) >
               first_iter->original_offset) {
      // |first_iter| comes before the |adjusted_iter| (as adjusted by |shift|).

      // It's not possible for the adjustments to overlap.  (It shouldn't
      // be possible that we have an |adjusted_iter->original_offset| that,
      // when adjusted by the computed |shift|, is in the middle of
      // |first_iter|'s output's length.  After all, that would mean the
      // current adjustment_on_adjusted_string somehow points to an offset
      // that was supposed to have been eliminated by the first set of
      // adjustments.)
      BASE_DCHECK(first_iter->original_offset + first_iter->output_length <=
                  adjusted_iter->original_offset + shift);

      // Add the |first_iter| to the full set of adjustments.
      shift += first_iter->original_length - first_iter->output_length;
      adjustments_builder.push_back(*first_iter);
      ++first_iter;
    } else {
      // The first adjustment adjusted something that then got further adjusted
      // by the second set of adjustments.  In other words, |first_iter| points
      // to something in the range covered by |adjusted_iter|'s length (after
      // accounting for |shift|).  Precisely,
      //   adjusted_iter->original_offset + shift
      //   <=
      //   first_iter->original_offset
      //   <=
      //   adjusted_iter->original_offset + shift +
      //       adjusted_iter->original_length

      // Modify the current |adjusted_iter| to include whatever collapsing
      // happened in |first_iter|, then advance to the next |first_adjustments|
      // because we dealt with the current one.
      const int collapse = static_cast<int>(first_iter->original_length) -
                           static_cast<int>(first_iter->output_length);
      // This function does not know how to deal with a string that expands and
      // then gets modified, only strings that collapse and then get modified.
      BASE_DCHECK(collapse > 0);
      adjusted_iter->original_length += collapse;
      currently_collapsing += collapse;
      ++first_iter;
    }
  }
  BASE_DCHECK(0u == currently_collapsing);
  if (first_iter != first_adjustments.end()) {
    // Only first adjustments are left.  These do not need to be modified.
    // (Their offsets are already correct with respect to the original string.)
    // Append them all.
    BASE_DCHECK(adjusted_iter == adjustments_on_adjusted_string->end());
    adjustments_builder.insert(adjustments_builder.end(), first_iter,
                               first_adjustments.end());
  }
  *adjustments_on_adjusted_string = std::move(adjustments_builder);
}

}  // namespace base
