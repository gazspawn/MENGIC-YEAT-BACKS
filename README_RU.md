# MENGIC - YEAT BACKS (VST3 source project)

Проект JUCE для сборки VST3-плагина под Windows / FL Studio.

## Что внутри
- Pitch: гранулярный pitch-shift с диапазоном -12..+12 полутонов
- Formant: дополнительная подчёркивающая верх/середину окраска (приближённый формантный эффект, не полноценный formant-preserving shifter)
- Drive: мягкий клиппинг / сатурация
- Width: Mid/Side ширина
- Delay + Feedback
- Reverb
- Mix и Output
- Сохранение параметров проекта/пресета хостом

## Сборка VST3 в Windows
1. Установите **Visual Studio 2022** (рабочая нагрузка «Разработка классических приложений на C++»), **CMake** и **Git**.
2. Откройте PowerShell в папке проекта.
3. Выполните:

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

CMake скачает JUCE 8.0.6 из официального репозитория. Для этого при первой сборке нужен интернет.

4. VST3 появится примерно здесь:
`build/MENGIC_YEAT_BACKS_artefacts/Release/VST3/MENGIC - YEAT BACKS.vst3`

5. Скопируйте папку `.vst3` в стандартную папку VST3:
`C:\Program Files\Common Files\VST3\`

6. В FL Studio откройте **Options → Manage plugins → Find installed plugins** и найдите `MENGIC - YEAT BACKS`.

## Стартовые настройки
Для широких бэков/адлибов начните с:
- Pitch: +5 semitones
- Formant: 25–35%
- Drive: 25–40%
- Width: 140–170%
- Delay: 10–25%
- Feedback: 20–35%
- Reverb: 15–30%
- Mix: 65–80%

На основном вокале уменьшайте Mix; лучше всего использовать на отдельном дубле/адлибах.

## Важно
Это исходный проект, а не уже собранный бинарный `.vst3`. Для компиляции нужны Windows, Visual Studio, CMake, Git и интернет для загрузки JUCE. Эффект — оригинальная обработка в направлении современного rage-вокала, а не копия конкретной записи или цепочки артиста.
