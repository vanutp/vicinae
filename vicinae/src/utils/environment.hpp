#pragma once
#include "vicinae.hpp"
#include <QString>
#include <QProcessEnvironment>
#include <QDir>
#include <cstdlib>
#include <filesystem>

namespace Environment {

inline bool isGnomeEnvironment() {
  const QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
  const QString session = qgetenv("GDMSESSION");
  return desktop.contains("GNOME", Qt::CaseInsensitive) || session.contains("gnome", Qt::CaseInsensitive);
}

inline bool isWaylandSession() { return QApplication::platformName() == "wayland"; }

/**
 * Detects if running in wlroots-based compositor (Hyprland, Sway, etc.)
 */
inline bool isWlrootsCompositor() {
  const QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
  return desktop.contains("Hyprland", Qt::CaseInsensitive) || desktop.contains("sway", Qt::CaseInsensitive) ||
         desktop.contains("river", Qt::CaseInsensitive);
}

inline bool isHudDisabled() { return qEnvironmentVariable("VICINAE_DISABLE_HUD", "0") == "1"; }

inline bool isLayerShellEnabled() { return qEnvironmentVariable("USE_LAYER_SHELL", "1") == "1"; }

/**
 * App image directory if we are running in an appimage.
 * We typically use this in order to find the bundled
 * node binary, instead of trying to launch the system one.
 */
inline std::optional<std::filesystem::path> appImageDir() {
  if (auto appdir = getenv("APPDIR")) return appdir;
  return std::nullopt;
}

/**
 * Optional override of the `node` executable to use to spawn the
 * extension manager.
 */
inline std::optional<std::filesystem::path> nodeBinaryOverride() {
  if (auto bin = getenv("NODE_BIN")) return bin;
  return std::nullopt;
}

inline bool isAppImage() { return appImageDir().has_value(); }

inline QStringList fallbackIconSearchPaths() {
  QStringList list;
  auto dirs = Omnicast::xdgDataDirs();

  list.reserve(dirs.size() * 2);

  for (const auto &dir : dirs) {
    list << (dir / "pixmaps").c_str();
  }

  for (const auto &dir : dirs) {
    list << (dir / "icons").c_str();
  }

  return list;
}

/**
 * Version of the Vicinae app.
 */
inline QString version() { return VICINAE_GIT_TAG; }

/**
 * Gets human-readable environment description
 */
inline QString getEnvironmentDescription() {
  QString desc;

  if (isGnomeEnvironment()) {
    desc = "GNOME";
  } else if (isWlrootsCompositor()) {
    desc = "wlroots";
  } else {
    const QString desktop = qgetenv("XDG_CURRENT_DESKTOP");
    desc = desktop.isEmpty() ? "Unknown" : desktop;
  }

  if (isWaylandSession()) {
    desc += "/Wayland";
  } else {
    desc += "/X11";
  }

  return desc;
}

/**
 * Returns a sanitized environment for launching external GUI apps so they don't inherit
 * Vicinae's Nix/Qt wrapper variables (which can crash apps like Electron on NixOS).
 *
 * We strip common wrapper vars and set a minimal PATH suitable for NixOS and non-Nix systems.
 * We keep important session vars (DISPLAY, WAYLAND_DISPLAY, XDG_RUNTIME_DIR, DBUS, etc.).
 */
inline QProcessEnvironment sanitizedAppLaunchEnvironment(const QString &program = QString()) {
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

  // Vars that frequently cause plugin/loader conflicts when inherited from wrapped Qt apps
  const QStringList toRemove = {
      "LD_LIBRARY_PATH",
      "LD_PRELOAD",
      "QT_PLUGIN_PATH",
      "QT_QPA_PLATFORM_PLUGIN_PATH",
      "QML2_IMPORT_PATH",
      "QML_IMPORT_PATH",
      "NIXPKGS_QT6_QML_IMPORT_PATH",
      "XDG_DATA_DIRS",
      "GSETTINGS_SCHEMA_DIR",
      "GST_PLUGIN_SYSTEM_PATH",
      "GST_PLUGIN_SYSTEM_PATH_1_0",
      "GST_PLUGIN_PATH"
  };

  for (const auto &k : toRemove) {
    if (env.contains(k)) env.remove(k);
  }

  QStringList cleanPath;
  cleanPath << QDir::homePath() + "/.local/bin" << "/run/wrappers/bin" << QDir::homePath() + "/.local/share/flatpak/exports/bin" << "/var/lib/flatpak/exports/bin" << QDir::homePath() + "/.nix-profile/bin" << "/nix/profile/bin" << QDir::homePath() + "/.local/state/nix/profile/bin" << "/etc/profiles/per-user/fox/bin" << "/nix/var/nix/profiles/default/bin" << "/run/current-system/sw/bin";

  env.insert("PATH", cleanPath.join(":"));

  return env;
}

} // namespace Environment
