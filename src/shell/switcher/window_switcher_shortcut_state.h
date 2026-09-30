#pragma once

#include "core/input/key_modifiers.h"
#include "core/log.h"

#include <cstdint>

class WindowSwitcherShortcutState {
public:
  static constexpr std::uint32_t kModifierMask = KeyMod::Ctrl | KeyMod::Alt | KeyMod::Super;
  static constexpr Logger kLog{"WindowSwitcherShortcutState"};

  void reset() noexcept {
    kLog.warn("reset()");
    m_shortcutModifiers = 0;
    m_pendingReleaseModifiers = 0;
    m_currentModifiers = 0;
    m_releaseCheckPending = false;
    m_focusCheckPending = false;
  }

  void capture(std::uint32_t modifiers) noexcept {
    kLog.warn("capture({})", modifiers);
    if (m_shortcutModifiers == 0) {
      m_shortcutModifiers = modifiers & kModifierMask;
    }
  }

  [[nodiscard]] std::uint32_t modifiers() const noexcept {
    kLog.warn("modifiers() -> {}", m_shortcutModifiers);
    return m_shortcutModifiers;
  }

  // A keybind opened the switcher while holding a shortcut modifier that may already be
  // released by the time the overlay gains keyboard focus, so that release never arrives.
  // The first release check after focus resolves it from the modifiers held at that point.
  void expectHeldModifier() noexcept {
    kLog.warn("expectHeldModifier()");
    m_focusCheckPending = true;
  }

  [[nodiscard]] bool hasPendingFocusCheck() const noexcept {
    kLog.warn("hasPendingFocusCheck() -> {}", m_focusCheckPending);
    return m_focusCheckPending;
  }

  [[nodiscard]] bool noteRelease(std::uint32_t releasedModifier, std::uint32_t currentModifiers) noexcept {
    capture(currentModifiers);
    capture(releasedModifier);
    const std::uint32_t matchingRelease = releasedModifier & m_shortcutModifiers;
    if (matchingRelease == 0) {
      kLog.warn("noteRelease({}, {}) -> false", releasedModifier, currentModifiers);
      return false;
    }
    m_pendingReleaseModifiers |= matchingRelease;
    kLog.warn("noteRelease({}, {}) -> true", releasedModifier, currentModifiers);
    updateModifiers(currentModifiers);
    return true;
  }

  void updateModifiers(std::uint32_t modifiers) noexcept {
    kLog.warn("updateModifiers({})", modifiers);
    m_currentModifiers = modifiers & kModifierMask;
  }

  [[nodiscard]] bool hasPendingRelease() const noexcept {
    kLog.warn("hasPendingRelease() -> {}", m_pendingReleaseModifiers != 0);
    return m_pendingReleaseModifiers != 0;
  }

  [[nodiscard]] bool beginReleaseCheck() noexcept {
    if (m_releaseCheckPending || (!hasPendingRelease() && !m_focusCheckPending)) {
      kLog.warn("beginReleaseCheck() -> false");
      return false;
    }
    m_releaseCheckPending = true;
    kLog.warn("beginReleaseCheck() -> true");
    return true;
  }

  [[nodiscard]] bool completeReleaseCheck(std::uint32_t modifiers) noexcept {
    if (!m_releaseCheckPending) {
      kLog.warn("completeReleaseCheck({}) -> false (no release check pending)", modifiers);
      return false;
    }
    m_releaseCheckPending = false;
    updateModifiers(modifiers);
    if (m_focusCheckPending) {
      m_focusCheckPending = false;
      if (m_currentModifiers == 0) {
        kLog.warn("completeReleaseCheck({}) -> true (focus check with no current modifiers)", modifiers);
        return true;
      }
      capture(m_currentModifiers);
    }
    const bool result = (m_pendingReleaseModifiers & ~m_currentModifiers) != 0;
    kLog.warn("completeReleaseCheck({}) -> {}", modifiers, result);
    return result;
  }

private:
  std::uint32_t m_shortcutModifiers = 0;
  std::uint32_t m_pendingReleaseModifiers = 0;
  std::uint32_t m_currentModifiers = 0;
  bool m_releaseCheckPending = false;
  bool m_focusCheckPending = false;
};
