# Copilot instructions — Telegram CLI (tgcli)

Коротко: этот репозиторий — терминальный клиент Telegram для Linux (C++20, CMake, TDLib + FTXUI). Цель — сделать полноценный клиент с функциями (сообщения, редактирование, реакции, Stars, Gifts/NFT и т.д.).

Build / test / lint (how-to)
- One-time: ./scripts/install_deps.sh
- Build TDLib (one-time): ./scripts/build_tdlib.sh
- Full build (from repo root):
  mkdir -p build && cd build
  cmake .. -DTd_DIR=/path/to/deps/td-install/lib/cmake/Td
  cmake --build . --target tgcli -j$(nproc)
- Build single target (from build/):
  cmake --build . --target tgcli -- -j1
- Run: tgcli  (or ./build/tgcli)
- Tests: в репозитории тестов нет. Если добавляются — добавляйте target test (CTest) и документируйте как запускать одну конкретную тестовую единицу.
- Lint/format: не настроено. Рекомендуется clang-format/clang-tidy; если добавите — документируйте команды в этом файле.

High-level architecture
- src/app — главный оркестратор (App, Config, State, exteraGram integration).
- src/telegram — TDLib-wrapper: Client, Auth, Messages, Stars, Gifts.
- src/ui — FTXUI-панели: ChatList, ChatView, InputBar, StarsPanel, GiftsPanel, InfoPanel.
- deps/td-install — локальная инсталляция TDLib (скрипты в scripts/).
- CMake: find_package(Td REQUIRED) — указывайте Td_DIR при конфигурации.

Key conventions & rules for Copilot sessions
- Язык: C++20. Собирайте с CMake (см. выше). Не менять внешние зависимости в deps/ без явного указания.
- Изменения: делайте маленькие атомарные изменения в отдельных ветках/PR.
- Commit messages: пользователь просит короткие, простые метки (последовательные номера). Для совместимости и откатимости:
  - Перед крупными изменениями создавайте тег или ветку со стабильной версией, например:
    git tag -a working-<date> -m "working snapshot"
    git push origin --tags
    или создайте ветку: git checkout -b stable
  - Для мелких шагов можно использовать простые сообщения: "1", "2" или "step 1: fix X". Но всегда делайте push и теги перед рискованными правками.
- Откат: чтобы вернуться к рабочей версии:
  - git checkout working-<date>  (если использован тег) или
  - git checkout stable
  - или git revert <bad-commit> для отмены конкретного коммита
- При внесении изменений в интеграцию с TDLib: запрашивайте подтверждение у автора/пользователя — изменения в TD-интерфейсе чувствительны.

Guidance for AI-generated patches
- Создавать мелкие PR с описанием: что изменено и как проверить.
- Не генерировать или изменять бинарные артефакты (build/, deps/td-install/*) — только исходники и скрипты.
- Включать в коммит трейлер Co-authored-by, если автоматические коммиты создаются автоматом.

Где смотреть
- README.md — основной гайд по установке и использованию.
- scripts/ — скрипты установки зависимостей и сборки TDLib.

Если нужно, могу сделать краткий пример workflow (GitHub Actions) для сборки и smoke-теста, добавить правила форматирования или зафиксировать процесс создания тегов/откатов.

