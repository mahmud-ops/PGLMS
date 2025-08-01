### 🧃 Steps (Windows)

[Video tutorial](https://youtu.be/yMmOjbp4yMI)

1. **Clone this repo**

   > Make sure to clone it somewhere easy to access — like your Desktop

   ```bash
   cd Desktop
   git clone https://github.com/mahmud-ops/OpenGL_setUP
   ```

2. **Open the folder in VSCode**

   * Right-click the `OpenGL_setup` folder
   * Hit **"Open with Code"**
     *(or from terminal: `code OpenGL_setup`)*

3. **Compile using this command** *(from inside the project root)*

   ```bash
   g++ src/main.cpp src/glad.c -Iinclude -Llib -lglfw3 -lopengl32 -lgdi32 -o app.exe
   ```

4. **Run it**

   * Double-click `app.exe`
   * Or use terminal: `./app.exe`

5. 💙 **Blue window = success**
   You’re officially OpenGL-activated.
