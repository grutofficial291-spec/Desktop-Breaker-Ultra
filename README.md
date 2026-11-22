# 💥 Desktop Breaker Ultra (Alpha v0.1)

![Language](https://shields.io)
![Library](https://shields.io)
![Platform](https://shields.io)
![Status](https://shields.io)
![PRs Welcome](https://shields.io)

> 🇷🇺 **[Русская версия описания находится ниже](#-desktop-breaker-ultra-alpha-v01-1)**

**Desktop Breaker Ultra** is a lightweight, chaotic desktop destruction sandbox written in pure **C + Raylib**. It captures your live Windows desktop and turns it into a destructible physics playground with 9 weapons, procedural real-time audio synthesis, multi-threaded SPH fluid simulation, and custom GLSL refraction shaders.

*(Built via vibe-coding — contributions, optimizations, and Pull Requests are very welcome!)*

---

### 🎮 Features & Arsenal

| Key | Weapon / Tool | Description |
| :---: | :--- | :--- |
| **`1`** | **Shotgun** | Blasts chunks off your screen with procedural cracks and sparks. |
| **`2`** | **Flamethrower** | Scorches icons, sets tiles on fire, and emits rising smoke. |
| **`3`** | **Plasma Laser** | High-energy continuous beam from the bottom of the screen. |
| **`4`** | **Black / White Hole** | Sucks in debris and water. Overloads after **20s** (or 25 items) into a **White Hole**, blasting everything back out! |
| **`5`** | **Frag Grenade** | Bounces off fallen screen tiles and detonates with a shockwave. |
| **`6`** | **Tsar Bomba (Nuke)** | Drops from the sky with a falling whistle, white flash, and mushroom cloud. |
| **`7`** | **Railgun** | Pierces the entire screen in a straight line with a hyper-velocity plasma bolt. |
| **`8`** | **Meteor Shower** | Calls down flaming meteors from above. |
| **`9`** | **Live Water (4-Core)** | Multi-threaded SPH water (up to 10,000 particles) with a **GLSL optical refraction shader** that bends icons underneath and bounces off desktop shortcuts. |

### ⌨️ Controls & Prank Hotkeys

* **`LMB`** — Fire / Use selected weapon
* **`RMB`** — Grab and throw broken desktop chunks
* **`R`** — Repair and reset the entire desktop
* **`Alt + U`** — **Stealth Mode:** Hides/shows the top UI bar and crosshair (restores standard Windows cursor)
* **`Alt + D`** — Minimizes all open windows and captures a clean desktop background
* **`F12`** — Toggle FPS & active water particle counter
* **`ESC`** — Exit

---

### 🛠️ Building from Source (TCC / MinGW)

The project is modularized into header-only addons (`addons.h` and `water.h`) and includes a custom GUI installer (`installer.c`) + icon generator (`make_icon.c`).

**1. Build the main game (using Tiny C Compiler):**
```bash
tcc main.c -I. -L. -lraylib -lgdi32 -luser32 -o breaker.exe
```

**2. (Optional) Generate icon & build the standalone installer:**
```bash
tcc make_icon.c -o make_icon.exe && make_icon.exe
tcc set_icon.c -lkernel32 -o set_icon.exe && set_icon.exe
tcc packer.c -o packer.exe && packer.exe
tcc installer.c -I. -L. -lraylib -lshell32 -lole32 -o DesktopBreaker_Setup.exe
set_icon.exe
```

---

### 🤝 Contributing (Help Wanted!)

This project is currently in **Alpha v0.1** and was created in a vibe-coding session. If you'd like to improve it, here are some great starting points:
* 🚀 **Water Physics Optimization:** Improve the 4-thread SPH solver (`water.h` / `addons.h`) to handle 15,000+ particles at locked 60 FPS even when compiled without `-O3`.
* 🖥️ **Multi-Monitor Support:** Extend screen capture and tile grid across multiple displays.
* 🐧 **Cross-Platform Port:** Add Linux (X11/Wayland) screen capture fallback.
* 🔫 **New Weapons:** Add a freeze ray, portal gun, or earthquake tool via modular `.h` hooks!

---
---

# 💥 Desktop Breaker Ultra (Alpha v0.1)

**Desktop Breaker Ultra** — это фановая физическая песочница-разрушитель рабочего стола, написанная на чистом **C + Raylib**. Программа делает снимок твоего рабочего стола и превращает его в разрушаемую площадку с 9 видами оружия, процедурным синтезатором звука (без внешних аудиофайлов), многопоточной водой на 4 ядра и GLSL-шейдером оптического преломления.

*(Проект создан в формате вайб-кодинга — любые улучшения кода, оптимизации и Pull Request'ы горячо приветствуются!)*

---

### 🎮 Арсенал и возможности

| Клавиша | Оружие / Инструмент | Описание |
| :---: | :--- | :--- |
| **`1`** | **Дробовик** | Выбивает куски экрана, оставляя паутину трещин по краям пробоин. |
| **`2`** | **Огнемёт** | Обугливает и поджигает иконки, поднимая густой дым. |
| **`3`** | **Плазменный лазер** | Мощный непрерывный луч из нижней части экрана. |
| **`4`** | **Чёрная / Белая дыра** | Засасывает осколки и воду. Через **20 секунд** (или при поглощении 25 кусков) перегружается и превращается в **Белую дыру**, выстреливая всё обратно! |
| **`5`** | **Осколочная граната** | Отскакивает от упавших иконок и взрывается с ударной волной. |
| **`6`** | **Царь-бомба (Ядерный удар)** | Падает сверху со свистом, ослепляет экран вспышкой и поднимает ядерный гриб. |
| **`7`** | **Рельсотрон** | Мгновенно прошивает весь экран насквозь сверхмощным плазменным зарядом. |
| **`8`** | **Метеоритный дождь** | Обрушивает с неба раскалённые метеориты. |
| **`9`** | **Живая вода (4 ядра)** | Многопоточная жидкость (до 10 000 частиц) с **GLSL-шейдером преломления**, которая искажает ярлыки под собой и отскакивает от иконок рабочего стола. |

### ⌨️ Управление и горячие клавиши

* **`ЛКМ`** — Огонь / Использование выбранного инструмента
* **`ПКМ`** — Схватить и швырнуть оторванный кусок экрана
* **`R`** — Починить и восстановить весь рабочий стол
* **`Alt + U`** — **Режим пранка:** скрывает/показывает верхнюю панель и прицел (возвращает обычный курсор Windows)
* **`Alt + D`** — Сворачивает все открытые окна и делает снимок чистого рабочего стола
* **`F12`** — Показать/скрыть счётчик FPS и активных частиц воды
* **`ESC`** — Выход из программы

---

### 🛠️ Сборка из исходников (TCC / MinGW)

Проект построен на модульных заголовочных аддонах (`addons.h` и `water.h`), а также включает свой графический установщик (`installer.c`) и генератор иконки (`make_icon.c`).

**1. Сборка самой игры (через Tiny C Compiler):**
```bash
tcc main.c -I. -L. -lraylib -lgdi32 -luser32 -o breaker.exe
```

**2. (Опционально) Генерация иконки и сборка автономного установщика:**
```bash
tcc make_icon.c -o make_icon.exe && make_icon.exe
tcc set_icon.c -lkernel32 -o set_icon.exe && set_icon.exe
tcc packer.c -o packer.exe && packer.exe
tcc installer.c -I. -L. -lraylib -lshell32 -lole32 -o DesktopBreaker_Setup.exe
set_icon.exe
```

---

### 🤝 Как помочь проекту (Contributing)

Проект находится в стадии **Alpha v0.1**. Буду рад любым идеям и Pull Request'ам, особенно по следующим задачам:
* 🚀 **Оптимизация физики воды:** ускорение 4-поточного SPH-солвера (`water.h` / `addons.h`), чтобы 15 000+ частиц работали в стабильных 60 FPS при заполнении всего монитора.
* 🖥️ **Поддержка нескольких мониторов:** захват и разрушение сразу всех подключённых экранов.
* 🔫 **Новые пушки:** замораживающий луч, портальная пушка или землетрясение в виде отдельных `.h`-модулей.
