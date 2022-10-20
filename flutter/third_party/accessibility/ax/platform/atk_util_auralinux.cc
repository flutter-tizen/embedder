// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atk/atk.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "atk_util_auralinux.h"
#include "ax_platform_node.h"

namespace {

//
// AtkUtilAuraLinux definition and implementation.
//
struct AtkUtilAuraLinux {
  AtkUtil parent;
};

struct AtkUtilAuraLinuxClass {
  AtkUtilClass parent_class;
};

G_DEFINE_TYPE(AtkUtilAuraLinux, atk_util_auralinux, ATK_TYPE_UTIL)

static void atk_util_auralinux_init(AtkUtilAuraLinux* ax_util) {}

static AtkObject* AtkUtilAuraLinuxGetRoot() {
  ui::AXPlatformNode* application = ui::AXPlatformNodeAuraLinux::application();
  if (application) {
    return application->GetNativeViewAccessible();
  }
  return nullptr;
}

using KeySnoopFuncMap = std::map<guint, std::pair<AtkKeySnoopFunc, gpointer>>;
static KeySnoopFuncMap& GetActiveKeySnoopFunctions() {
  static base::NoDestructor<KeySnoopFuncMap> active_key_snoop_functions;
  return *active_key_snoop_functions;
}

using AXPlatformNodeSet = std::set<ui::AXPlatformNodeAuraLinux*>;
static AXPlatformNodeSet& GetNodesWithPostponedEvents() {
  static base::NoDestructor<AXPlatformNodeSet> nodes_with_postponed_events_list;
  return *nodes_with_postponed_events_list;
}

static void RunPostponedEvents() {
  for (ui::AXPlatformNodeAuraLinux* entry : GetNodesWithPostponedEvents()) {
    entry->RunPostponedEvents();
  }
  GetNodesWithPostponedEvents().clear();
}

static guint AtkUtilAuraLinuxAddKeyEventListener(
    AtkKeySnoopFunc key_snoop_function,
    gpointer data) {
  static guint current_key_event_listener_id = 0;

  // AtkUtilAuraLinuxAddKeyEventListener is called by
  // spi_atk_register_event_listeners in the at-spi2-atk library after it has
  // initialized all its other listeners. Our internal knowledge of the
  // internals of the AT-SPI/ATK bridge allows us to use this call as an
  // indication of AT-SPI being ready, so we can finally run any events that had
  // been waiting.
  ui::AtkUtilAuraLinux::GetInstance().SetAtSpiReady(true);

  current_key_event_listener_id++;
  GetActiveKeySnoopFunctions()[current_key_event_listener_id] =
      std::make_pair(key_snoop_function, data);
  return current_key_event_listener_id;
}

static void AtkUtilAuraLinuxRemoveKeyEventListener(guint listener_id) {
  GetActiveKeySnoopFunctions().erase(listener_id);
}

static void atk_util_auralinux_class_init(AtkUtilAuraLinuxClass* klass) {
  AtkUtilClass* atk_class = ATK_UTIL_CLASS(g_type_class_peek(ATK_TYPE_UTIL));

  atk_class->get_root = AtkUtilAuraLinuxGetRoot;
  atk_class->get_toolkit_name = []() { return "Chromium"; };
  atk_class->get_toolkit_version = []() { return "1.0"; };
  atk_class->add_key_event_listener = AtkUtilAuraLinuxAddKeyEventListener;
  atk_class->remove_key_event_listener = AtkUtilAuraLinuxRemoveKeyEventListener;
}

}  // namespace

namespace ui {

// static
AtkUtilAuraLinux& AtkUtilAuraLinux::GetInstance() {
  static AtkUtilAuraLinux AtkUtilInstance;
  return AtkUtilInstance;
}

void AtkUtilAuraLinux::InitializeAsync() {
  static bool initialized = false;

  if (initialized)
    return;

  initialized = true;

  // Register our util class.
  g_type_class_unref(g_type_class_ref(atk_util_auralinux_get_type()));

  PlatformInitializeAsync();
}

bool AtkUtilAuraLinux::IsAtSpiReady() {
  return at_spi_ready_;
}

void AtkUtilAuraLinux::SetAtSpiReady(bool ready) {
  at_spi_ready_ = ready;
  if (ready)
    RunPostponedEvents();
}

void AtkUtilAuraLinux::PostponeEventsFor(AXPlatformNodeAuraLinux* node) {
  GetNodesWithPostponedEvents().insert(node);
}

void AtkUtilAuraLinux::CancelPostponedEventsFor(AXPlatformNodeAuraLinux* node) {
  GetNodesWithPostponedEvents().erase(node);
}

}  // namespace ui
