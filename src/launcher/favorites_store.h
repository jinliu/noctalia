#pragma once

#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>

class FavoritesStore {
public:
  FavoritesStore();

  void add(std::string_view providerId, std::string_view resultId);
  void remove(std::string_view providerId, std::string_view resultId);
  void moveUp(std::string_view providerId, std::string_view resultId);
  void moveDown(std::string_view providerId, std::string_view resultId);
  // Returns the position of the favorite, larger is closer to the front. Returns 0 if not a favorite.
  [[nodiscard]] int getPos(std::string_view providerId, std::string_view resultId) const;
  [[nodiscard]] std::deque<std::string> getFavorites(std::string_view providerId) const;
  [[nodiscard]] std::size_t getFavoritesCount(std::string_view providerId) const;

private:
  void load();
  void save() const;
  void rebuildIndex(std::string_view provider);

  std::string m_favoritesPath;
  std::unordered_map<std::string, std::deque<std::string>> m_favorites;
  std::unordered_map<std::string, std::unordered_map<std::string, int>> m_favoritesIndex;
};
