// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_STRING_UTILS_H_
#define BASE_STRING_UTILS_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace base {

constexpr char16_t kWhitespaceUTF16 = u' ';

// Return a C++ string given printf-like input.
template <typename... Args>
std::string StringPrintf(const std::string& format, Args... args) {
  // Calculate the buffer size.
  int size = snprintf(nullptr, 0, format.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf = std::make_unique<char[]>(size);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

std::u16string ASCIIToUTF16(std::string src);
std::u16string UTF8ToUTF16(std::string src);
std::string UTF16ToUTF8(std::u16string src);
std::u16string WideToUTF16(const std::wstring& src);
std::wstring UTF16ToWide(const std::u16string& src);

std::u16string NumberToString16(unsigned int number);
std::u16string NumberToString16(int32_t number);
std::u16string NumberToString16(float number);
std::u16string NumberToString16(double number);

std::string NumberToString(unsigned int number);
std::string NumberToString(int32_t number);
std::string NumberToString(float number);
std::string NumberToString(double number);
std::string NumberToString(int64_t number);

std::string ToUpperASCII(std::string str);
std::string ToLowerASCII(std::string str);

std::string JoinString(std::vector<std::string> tokens, std::string delimiter);
std::u16string JoinString(std::vector<std::u16string> tokens,
                          std::u16string delimiter);

void ReplaceChars(std::string in,
                  std::string from,
                  std::string to,
                  std::string* out);
void ReplaceChars(std::u16string in,
                  std::u16string from,
                  std::u16string to,
                  std::u16string* out);
bool LowerCaseEqualsASCII(std::string a, std::string b);
bool ContainsOnlyChars(std::u16string str, char16_t ch);

const std::string& EmptyString();

enum class CompareCase {
  SENSITIVE,
  INSENSITIVE_ASCII,
};

bool StartsWith(std::string str,
                std::string search_for,
                CompareCase case_sensitivity = CompareCase::SENSITIVE);
bool EndsWith(std::string str,
              std::string search_for,
              CompareCase case_sensitivity = CompareCase::SENSITIVE);

inline bool IsValidCodepoint(uint32_t code_point) {
  // Excludes code points that are not Unicode scalar values, i.e.
  // surrogate code points ([0xD800, 0xDFFF]). Additionally, excludes
  // code points larger than 0x10FFFF (the highest codepoint allowed).
  // Non-characters and unassigned code points are allowed.
  // https://unicode.org/glossary/#unicode_scalar_value
  return code_point < 0xD800u ||
         (code_point >= 0xE000u && code_point <= 0x10FFFFu);
}

// Reads a UTF-8 stream, placing the next code point into the given output
// |*code_point|. |src| represents the entire string to read, and |*char_index|
// is the character offset within the string to start reading at. |*char_index|
// will be updated to index the last character read, such that incrementing it
// (as in a for loop) will take the reader to the next character.
//
// Returns true on success. On false, |*code_point| will be invalid.
bool ReadUnicodeCharacter(const char* src,
                          int32_t src_len,
                          int32_t* char_index,
                          uint32_t* code_point_out);

// Reads a UTF-16 character. The usage is the same as the 8-bit version above.
bool ReadUnicodeCharacter(const char16_t* src,
                          int32_t src_len,
                          int32_t* char_index,
                          uint32_t* code_point);

// A helper class and associated data structures to adjust offsets into a
// string in response to various adjustments one might do to that string
// (e.g., eliminating a range).  For details on offsets, see the comments by
// the AdjustOffsets() function below.
class OffsetAdjuster {
 public:
  struct Adjustment {
    Adjustment(size_t original_offset,
               size_t original_length,
               size_t output_length);

    size_t original_offset;
    size_t original_length;
    size_t output_length;
  };
  typedef std::vector<Adjustment> Adjustments;

  // Adjusts all offsets in |offsets_for_adjustment| to reflect the adjustments
  // recorded in |adjustments|.  Adjusted offsets greater than |limit| will be
  // set to std::u16string::npos.
  //
  // Offsets represents insertion/selection points between characters: if |src|
  // is "abcd", then 0 is before 'a', 2 is between 'b' and 'c', and 4 is at the
  // end of the string.  Valid input offsets range from 0 to |src_len|.  On
  // exit, each offset will have been modified to point at the same logical
  // position in the output string.  If an offset cannot be successfully
  // adjusted (e.g., because it points into the middle of a multibyte sequence),
  // it will be set to std::u16string::npos.
  static void AdjustOffsets(const Adjustments& adjustments,
                            std::vector<size_t>* offsets_for_adjustment,
                            size_t limit = std::u16string::npos);

  // Adjusts the single |offset| to reflect the adjustments recorded in
  // |adjustments|.
  static void AdjustOffset(const Adjustments& adjustments,
                           size_t* offset,
                           size_t limit = std::u16string::npos);

  // Adjusts all offsets in |offsets_for_unadjustment| to reflect the reverse
  // of the adjustments recorded in |adjustments|.  In other words, the offsets
  // provided represent offsets into an adjusted string and the caller wants
  // to know the offsets they correspond to in the original string.  If an
  // offset cannot be successfully unadjusted (e.g., because it points into
  // the middle of a multibyte sequence), it will be set to
  // std::u16string::npos.
  static void UnadjustOffsets(const Adjustments& adjustments,
                              std::vector<size_t>* offsets_for_unadjustment);

  // Adjusts the single |offset| to reflect the reverse of the adjustments
  // recorded in |adjustments|.
  static void UnadjustOffset(const Adjustments& adjustments, size_t* offset);

  // Combines two sequential sets of adjustments, storing the combined revised
  // adjustments in |adjustments_on_adjusted_string|.  That is, suppose a
  // string was altered in some way, with the alterations recorded as
  // adjustments in |first_adjustments|.  Then suppose the resulting string is
  // further altered, with the alterations recorded as adjustments scored in
  // |adjustments_on_adjusted_string|, with the offsets recorded in these
  // adjustments being with respect to the intermediate string.  This function
  // combines the two sets of adjustments into one, storing the result in
  // |adjustments_on_adjusted_string|, whose offsets are correct with respect
  // to the original string.
  //
  // Assumes both parameters are sorted by increasing offset.
  //
  // WARNING: Only supports |first_adjustments| that involve collapsing ranges
  // of text, not expanding ranges.
  static void MergeSequentialAdjustments(
      const Adjustments& first_adjustments,
      Adjustments* adjustments_on_adjusted_string);
};

}  // namespace base

#endif  // BASE_STRING_UTILS_H_
