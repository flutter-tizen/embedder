// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_MAPPING_H_
#define EMBEDDER_KEY_MAPPING_H_

#include <cstdint>
#include <map>
#include <string>

// Maps XKB scan codes to GTK key codes.
//
// The values are originally defined in:
// - flutter/keyboard_maps.dart (kLinuxToPhysicalKey)
// - flutter/keyboard_maps.dart (kGtkToLogicalKey)
//
// Provided only for backward compatibility. This will be removed after the
// legacy RawKeyboard API is removed from the framework in the future.
extern std::map<uint32_t, uint32_t> kScanCodeToGtkKeyCode;

// Maps Ecore modifiers to GTK modifiers.
//
// The values are originally defined in:
// - efl/Ecore_Input.h
// - flutter/raw_keyboard_linux.dart (GtkKeyHelper)
//
// Provided only for backward compatibility. This will be removed after the
// legacy RawKeyboard API is removed from the framework in the future.
extern std::map<int, int> kEcoreModifierToGtkModifier;

// Maps XKB scan codes to Flutter's physical key codes.
//
// This is a copy of the Linux embedder's |xkb_to_physical_key_map|.
extern std::map<uint32_t, uint64_t> kScanCodeToPhysicalKeyCode;

// Maps Tizen key symbols to Flutter's logical key codes.
extern std::map<std::string, uint64_t> kSymbolToLogicalKeyCode;

// Mask for the 32-bit value portion of the key code.
const uint64_t kValueMask = 0x000ffffffff;

// The plane value for private keys defined by the Tizen platform.
const uint64_t kTizenPlane = 0x02000000000;

#endif  // EMBEDDER_KEY_MAPPING_H_
