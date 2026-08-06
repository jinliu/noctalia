#include "launcher/favorites_store.h"

#include "core/log.h"
#include "util/file_utils.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
  constexpr Logger kLog("launcher.favorites-store");
}

FavoritesStore::FavoritesStore() {
  const std::string dir = FileUtils::stateDir();
  m_favoritesPath = (dir.empty() ? "." : dir) + "/favorites.json";
  load();
}

void FavoritesStore::add(std::string_view providerId, std::string_view resultId) {
  if (getPos(providerId, resultId) > 0) {
    return;
  }
  m_favorites[std::string(providerId)].push_back(std::string(resultId));
  rebuildIndex(providerId);
  save();
}

void FavoritesStore::remove(std::string_view providerId, std::string_view resultId) {
  if (getPos(providerId, resultId) == 0) {
    return;
  }
  auto& favorites = m_favorites[std::string(providerId)];
  favorites.erase(std::remove(favorites.begin(), favorites.end(), std::string(resultId)), favorites.end());
  if (favorites.empty()) {
    m_favorites.erase(std::string(providerId));
    m_favoritesIndex.erase(std::string(providerId));
  } else {
    rebuildIndex(providerId);
  }
  save();
}

void FavoritesStore::moveUp(std::string_view providerId, std::string_view resultId) {
  auto& favorites = m_favorites[std::string(providerId)];
  auto it = std::find(favorites.begin(), favorites.end(), std::string(resultId));
  if (it != favorites.end() && it != favorites.begin()) {
    std::swap(*it, *(it - 1));
    rebuildIndex(providerId);
    save();
  }
}

void FavoritesStore::moveDown(std::string_view providerId, std::string_view resultId) {
  auto& favorites = m_favorites[std::string(providerId)];
  auto it = std::find(favorites.begin(), favorites.end(), std::string(resultId));
  if (it != favorites.end() && (it + 1) != favorites.end()) {
    std::swap(*it, *(it + 1));
    rebuildIndex(providerId);
    save();
  }
}

int FavoritesStore::getPos(std::string_view providerId, std::string_view resultId) const {
  const auto provIt = m_favoritesIndex.find(std::string(providerId));
  if (provIt == m_favoritesIndex.end()) {
    return 0;
  }
  const auto idIt = provIt->second.find(std::string(resultId));
  return idIt != provIt->second.end() ? idIt->second : 0;
}

std::deque<std::string> FavoritesStore::getFavorites(std::string_view providerId) const {
  const auto provIt = m_favorites.find(std::string(providerId));
  if (provIt == m_favorites.end()) {
    return {};
  }
  return provIt->second;
}

std::size_t FavoritesStore::getFavoritesCount(std::string_view providerId) const {
  const auto provIt = m_favorites.find(std::string(providerId));
  return provIt != m_favorites.end() ? provIt->second.size() : 0;
}

void FavoritesStore::load() {
  std::ifstream file(m_favoritesPath);
  if (file.is_open()) {
    try {
      const auto json = nlohmann::json::parse(file);
      for (const auto& [provider, ids] : json.items()) {
        m_favorites[provider].assign(ids.begin(), ids.end());
        rebuildIndex(provider);
      }
    } catch (const nlohmann::json::exception&) {
      // Ignore malformed file — starts fresh
    }
  }
}

void FavoritesStore::save() const {
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(m_favoritesPath).parent_path(), ec);
  if (ec) {
    kLog.error("Failed to create directory {}, error: {}", m_favoritesPath, ec.message());
    return;
  }
  nlohmann::json json = m_favorites;
  std::ofstream file(m_favoritesPath, std::ios::trunc);
  file << json.dump(2) << '\n';
  file.close();
  if (!file) {
    kLog.error("Failed to save favorites to {}", m_favoritesPath);
  }
}

void FavoritesStore::rebuildIndex(std::string_view provider) {
  const auto& favorites = m_favorites[std::string(provider)];
  auto& index = m_favoritesIndex[std::string(provider)];
  index.clear();
  for (std::size_t i = 0; i < favorites.size(); ++i) {
    index[favorites[i]] = static_cast<int>(favorites.size() - i);
  }
}
