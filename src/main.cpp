#include <unordered_set>
#include "Log.hpp"
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/helpers/WLClasses.hpp>

#include "Globals.hpp"
#include <hyprland/src/plugins/PluginAPI.hpp>

#include <hyprland/src/debug/Log.hpp>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/managers/AnimationManager.hpp>

#ifdef FLASH
#include "Flash.hpp"
#endif
#ifdef SHRINK
#include "Shrink.hpp"
#endif

PHLWINDOW g_pPreviouslyFocusedWindow = nullptr;
PHLWINDOW g_pPreviouslyClosedWindow = nullptr;
// HACK: This is a hack to keep track of all opened windows,
// since the openWindow event is called after the activeWindow.
std::unordered_set<PHLWINDOW> g_OpenedWindows = {};

bool g_bMouseWasPressed = false;
bool g_bWorkspaceChanged = false;

std::unordered_map<std::string, std::unique_ptr<IFocusAnimation>> g_mAnimations;

static bool OnSameWorkspace(PHLWINDOW pWindow1, PHLWINDOW pWindow2) {
  if (pWindow1 == pWindow2) {
    return true;
  } else if (pWindow1 == nullptr || pWindow2 == nullptr) {
    return false;
  } else if (pWindow1->workspaceID() == pWindow2->workspaceID()) {
    return true;
  } else {
    return false;
  }
}

void flashWindow(PHLWINDOW pWindow) {
  // static const Hyprlang::STRING *focusAnimation = nullptr;
  // if (g_bMouseWasPressed == true) {
  //   hyprfocus_log(LOG, "Mouse was pressed");
  //   focusAnimation =
  //       (Hyprlang::STRING const *)(HyprlandAPI::getConfigValue(
  //                                      PHANDLE,
  //                                      "plugin:hyprfocus:mouse_focus_animation")
  //                                      ->getDataStaticPtr());
  // } else {
  //   focusAnimation =
  //       (Hyprlang::STRING const
  //            *)(HyprlandAPI::getConfigValue(
  //                   PHANDLE, "plugin:hyprfocus:keyboard_focus_animation")
  //                   ->getDataStaticPtr());
  // }
  static const Hyprlang::STRING *focusAnimation =
      (Hyprlang::STRING const *)(HyprlandAPI::getConfigValue(
                                     PHANDLE,
                                     "plugin:hyprfocus:focus_animation")
                                     ->getDataStaticPtr());
  hyprfocus_log(LOG, "Flashing window");
  hyprfocus_log(LOG, "Animation: {}", *focusAnimation);

  auto it = g_mAnimations.find(*focusAnimation);
  if (it != g_mAnimations.end()) {
    hyprfocus_log(LOG, "Calling onWindowFocus for animation {}",
                  *focusAnimation);
    it->second->onWindowFocus(pWindow, PHANDLE);
  }
}

#ifdef DISPATCH
void flashCurrentWindow(std::string) {
  hyprfocus_log(LOG, "Flashing current window");
  static auto *const PHYPRFOCUSENABLED =
      (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
          PHANDLE, "plugin:hyprfocus:enabled")
          ->getDataStaticPtr();
  if (!*PHYPRFOCUSENABLED) {
    hyprfocus_log(LOG, "HyprFocus is disabled");
    return;
  }

  if (g_pPreviouslyFocusedWindow == nullptr) {
    hyprfocus_log(LOG, "No previously focused window");
    return;
  }

  flashWindow(g_pPreviouslyFocusedWindow);
}
#endif

static void onActiveWindowChange(void *self, std::any data) {
  try {
    hyprfocus_log(LOG, "Active window changed");
    auto const PWINDOW = std::any_cast<PHLWINDOW>(data);
    static auto *const PHYPRFOCUSENABLED =
        (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
            PHANDLE, "plugin:hyprfocus:enabled")
            ->getDataStaticPtr();

    static auto *const PANIMATEFLOATING =
        (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
            PHANDLE, "plugin:hyprfocus:animate_floating")
            ->getDataStaticPtr();

    static auto *const PANIMATEWORKSPACECHANGE =
        (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
            PHANDLE, "plugin:hyprfocus:animate_workspacechange")
            ->getDataStaticPtr();

    static auto *const PMOUSEENABLED =
        (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
            PHANDLE, "plugin:hyprfocus:mouse_enabled")
            ->getDataStaticPtr();

    static auto *const PANIMATECOUSEDBYCLOSE =
        (Hyprlang::INT *const *)HyprlandAPI::getConfigValue(
            PHANDLE, "plugin:hyprfocus:animate_coused_by_close")
            ->getDataStaticPtr();

    if (!**PHYPRFOCUSENABLED) {
      hyprfocus_log(LOG, "HyprFocus is disabled");
      return;
    }

    if (PWINDOW == nullptr) {
      hyprfocus_log(LOG, "Window is null");
      return;
    }

    if (PWINDOW == g_pPreviouslyFocusedWindow) {
      hyprfocus_log(LOG, "Window is the same as the previous one");
      return;
    }

    if (g_OpenedWindows.find(PWINDOW) == g_OpenedWindows.end()) {
      hyprfocus_log(LOG, "Window is newly opened");
      g_OpenedWindows.insert(PWINDOW);
      g_pPreviouslyFocusedWindow = PWINDOW;
      return;
    }

    if (!**PANIMATECOUSEDBYCLOSE &&
        g_pPreviouslyFocusedWindow == g_pPreviouslyClosedWindow) {
      hyprfocus_log(LOG, "Previously focused window was closed, not animating");
      g_pPreviouslyFocusedWindow = PWINDOW;
      g_pPreviouslyClosedWindow = nullptr;
      return;
    }

    if (!**PMOUSEENABLED && g_bMouseWasPressed) {
      hyprfocus_log(LOG, "Mouse was pressed, not animating");
      g_pPreviouslyFocusedWindow = PWINDOW;
      return;
    }

    if (PWINDOW->m_bIsFloating && !**PANIMATEFLOATING) {
      hyprfocus_log(LOG, "Floating window, not animating");
      g_pPreviouslyFocusedWindow = PWINDOW;
      return;
    }

    if (!**PANIMATEWORKSPACECHANGE &&
        !OnSameWorkspace(PWINDOW, g_pPreviouslyFocusedWindow)) {
      hyprfocus_log(LOG, "Workspace changed, not animating");
      g_pPreviouslyFocusedWindow = PWINDOW;
      return;
    }

    flashWindow(PWINDOW);
    g_pPreviouslyFocusedWindow = PWINDOW;

  } catch (std::bad_any_cast &e) {
    hyprfocus_log(ERR, "Cast Error: {}", e.what());
  } catch (std::exception &e) {
    hyprfocus_log(ERR, "Error: {}", e.what());
  }
}

static void onMouseButton(void *self, std::any data) {
  try {
    auto const PWLRMOUSEBUTTONEVENT =
        std::any_cast<IPointer::SButtonEvent>(data);
    hyprfocus_log(LOG, "Mouse button state: {}",
                  (int)PWLRMOUSEBUTTONEVENT.state);
    g_bMouseWasPressed = (int)PWLRMOUSEBUTTONEVENT.state == 1;

  } catch (std::bad_any_cast &e) {
    hyprfocus_log(ERR, "Cast Error: {}", e.what());
  } catch (std::exception &e) {
    hyprfocus_log(ERR, "Error: {}", e.what());
  }
}

// static void onWindowOpen(void *self, std::any data) {
//   try {
//     auto const PWINDOW = std::any_cast<PHLWINDOW>(data);
//
//   } catch (std::bad_any_cast &e) {
//     hyprfocus_log(ERR, "Cast Error: {}", e.what());
//   } catch (std::exception &e) {
//     hyprfocus_log(ERR, "Error: {}", e.what());
//   }
// }

static void onWindowClose(void *self, std::any data) {
  try {
    auto const PWINDOW = std::any_cast<PHLWINDOW>(data);
    g_pPreviouslyClosedWindow = PWINDOW;
    g_OpenedWindows.erase(PWINDOW);
  } catch (std::bad_any_cast &e) {
    hyprfocus_log(ERR, "Cast Error: {}", e.what());
  } catch (std::exception &e) {
    hyprfocus_log(ERR, "Error: {}", e.what());
  }
}

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprfocus:enabled",
                              Hyprlang::INT{0});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprfocus:mouse_enabled",
                              Hyprlang::INT{0});
  HyprlandAPI::addConfigValue(
      PHANDLE, "plugin:hyprfocus:animate_workspacechange", Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprfocus:animate_floating",
                              Hyprlang::INT{1});
  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprfocus:focus_animation",
                              Hyprlang::STRING("none"));
  HyprlandAPI::addConfigValue(
      PHANDLE, "plugin:hyprfocus:animate_coused_by_close", Hyprlang::INT{0});
#ifdef DISPATCH
  HyprlandAPI::addDispatcher(PHANDLE, "animatefocused", &flashCurrentWindow);
#endif

#ifdef FLASH
  g_mAnimations["flash"] = std::make_unique<CFlash>();
#endif
#ifdef SHRINK
  g_mAnimations["shrink"] = std::make_unique<CShrink>();
#endif
  g_mAnimations["none"] = std::make_unique<IFocusAnimation>();

  for (auto &[name, pAnimation] : g_mAnimations) {
    pAnimation->init(PHANDLE, name);
    hyprfocus_log(LOG, "Registered animation: {}", name);
  }

  HyprlandAPI::reloadConfig();
  g_pConfigManager->tick();
  hyprfocus_log(LOG, "Reloaded config");

  // Register callbacks
  static auto P1 = HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "activeWindow",
      [&](void *self, SCallbackInfo &info, std::any data) {
        onActiveWindowChange(self, data);
      });
  hyprfocus_log(LOG, "Registered active window change callback");

  static auto P2 = HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "mouseButton",
      [&](void *self, SCallbackInfo &info, std::any data) {
        onMouseButton(self, data);
      });
  hyprfocus_log(LOG, "Registered mouse button callback");

  //  static auto P3 = HyprlandAPI::registerCallbackDynamic(
  //      PHANDLE, "openWindow",
  //      [&](void *self, SCallbackInfo &info, std::any data) {
  //        onWindowOpen(self, data);
  //      });
  //  hyprfocus_log(LOG, "Registered open window callback");

  static auto P4 = HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "closeWindow",
      [&](void *self, SCallbackInfo &info, std::any data) {
        onWindowClose(self, data);
      });
  hyprfocus_log(LOG, "Registered close window callback");

  HyprlandAPI::reloadConfig();

  return {"hyprfocus", "animate windows on focus", "Vortex", "2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
