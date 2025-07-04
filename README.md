# OpenGL Setup Starter Pack 🧰

A plug-and-play OpenGL + GLFW + GLAD setup for C++ projects — skip the hassle, run the code, and get a blue window like a boss 💙

---

## 📦 What's Inside

```bash
├── .vscode/         # VSCode config (optional)
├── include/         # Header files (GLAD, GLFW etc.)
├── lib/             # Precompiled GLFW & other libs
├── src/             # Your source files (main.cpp, glad.c)
├── glfw3.dll        # DLL needed for Windows runtime
├── cutable.exe      # Precompiled demo (blue window)
````

---

## 🧪 What It Does

* Opens a basic OpenGL window (800x600)
* Uses GLFW for window/context creation
* Uses GLAD for accessing OpenGL functions
* Just a background color? Yup — it's the Hello World of OpenGL

---

## 🚀 How to Run (Windows)

### 🛠 Requirements:

* A C++ compiler (MinGW / MSVC)
* VSCode or any IDE that supports C++
* All dependencies are included ✅

### 🧃 Steps:

1. Clone this repo

   ```bash
   git clone https://github.com/mahmud-ops/OpenGL_setup.git
   ```
2. Open in VSCode
3. Compile with:

   ```bash
   g++ src/main.cpp src/glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -o app.exe
   ```
4. Run `app.exe`
5. You should see a clean window with a chill blue background

---

## 💡 Pro Tip

You can use this as a **starter template** for:

* 2D games / UIs
* Learning OpenGL shaders
* Practicing vertex buffers & rendering
* Triangle supremacy 🛸

---

## 📬 Credits

Created by [mahmud-ops](https://github.com/mahmud-ops)
GLFW: [https://www.glfw.org](https://www.glfw.org)
GLAD: [https://glad.dav1d.de/](https://glad.dav1d.de/)

---

## 🔓 License

MIT (Do whatever you want, just don't say you made it if you didn't 😎)
