// 12-апта: екі текстураны бір объектіде біріктіру, blend коэффициентін uniform арқылы басқару
//
// 11-аптадағы текстураланған куб + еркін камера. loadTexture() сол қалпында.
// Басқару: W/A/S/D жылжу, Space/Left Shift жоғары/төмен, Left Ctrl жүгіру, тінтуір - бұрылу,
//          дөңгелек - zoom, Esc - шығу, Жоғары/Төмен - blend (dt-ге көбейтілген)
//   1 mix  2 a*b  3 a+b  4 clamp(a+b)  5 логотип (decal.a)  6 mix(a,b,uv.y)  7 smoothstep  8 үш текстура
//   L - екінші текстура: pattern <-> logo      B - blend автоматты (sin(time))
//   R - логотипті айналдыру (Қ4)               G - әр жаққа бөлек blend (Қ3)
//
// Тексеру флагтары (скриншоттар үшін):
//   --blend f   --mode 1..8   --tex2 pattern|logo   --broken-units (1-тапсырма)
//   --wrong-id (2-тапсырма)   --three (Қ1)   --per-face (Қ3)   --rotate (Қ4)
//   --cam x,y,z  --fov a  --shot f.ppm
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

// Позиция (3) + UV (2). 11-аптадағы массивпен бірдей: әр жақ - 6 вершина (Қ3 үшін f * 6).
float vertices[] = {
    // +Z алдыңғы
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.5f, -0.5f,  0.5f,  1.0f, 0.0f,   0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    // -Z артқы
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    // -X сол
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  -0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    // +X оң
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.5f, -0.5f, -0.5f,  1.0f, 0.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f,   0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    // +Y жоғары
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f,  0.5f,  1.0f, 0.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,   0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    // -Y төмен
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,   0.5f, -0.5f, -0.5f,  1.0f, 0.0f,   0.5f, -0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,   0.5f, -0.5f,  0.5f,  1.0f, 1.0f,  -0.5f, -0.5f,  0.5f,  0.0f, 1.0f,
};

glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f), glm::vec3( 2.0f,  0.5f, -2.0f), glm::vec3(-1.8f, -0.3f, -1.0f),
    glm::vec3( 1.2f, -0.8f, -3.5f), glm::vec3(-2.2f,  1.0f, -4.0f),
};

// ---- Камера ----
glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 3.0f), camFront = glm::vec3(0.0f, 0.0f, -1.0f), camUp = glm::vec3(0, 1, 0);
float yaw = -90.0f, pitch = 0.0f, fov = 45.0f, lastX = 400.0f, lastY = 300.0f;
bool firstMouse = true;
const int gWidth = 800, gHeight = 600;

// ---- 12-апта күйі ----
float blend = 0.5f;            // 5-қадам: жаһандық коэффициент
int   mode = 1;                // 1..8 - фрагмент шейдеріндегі араластыру тәсілі
bool  autoBlend = false;       // Қ2
bool  rotateLogo = false;      // Қ4
bool  perFace = false;         // Қ3
bool  useThree = false;        // Қ1
bool  brokenUnits = false;     // 1-тапсырма: glActiveTexture жолдары "комментарийде"
bool  wrongId = false;         // 2-тапсырма: glUniform1i-ге ID беру
bool  logoAsTex2 = false;
unsigned int texContainer = 0, texPattern = 0, texLogo = 0;
float simTime = -1.0f;         // --shot режимінде тұрақты уақыт

void updateFront() {
    glm::vec3 d;
    d.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    d.y = sin(glm::radians(pitch));
    d.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camFront = glm::normalize(d);
}
void mouse_callback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }
    yaw += ((float)xpos - lastX) * 0.1f;
    pitch += (lastY - (float)ypos) * 0.1f;
    lastX = (float)xpos; lastY = (float)ypos;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    updateFront();
}
void scroll_callback(GLFWwindow*, double, double yoffset) {
    fov -= (float)yoffset;
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 90.0f) fov = 90.0f;
}

void processInput(GLFWwindow* w, float dt) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    float speed = 2.5f * dt;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f;
    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) camPos += speed * glm::vec3(0, 1, 0);
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * glm::vec3(0, 1, 0);

    // 5-қадам: үздіксіз әрекет - жылдамдық * dt (5-жиі кететін қате: dt-ні ұмыту)
    if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS)   blend += 0.5f * dt;
    if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS) blend -= 0.5f * dt;
    if (blend > 1.0f) blend = 1.0f;
    if (blend < 0.0f) blend = 0.0f;

    // Бір басуға бір рет ауысатын күйлер (10-аптадағы әдіс: алдыңғы күйді сақтау)
    static bool prev[512] = {};
    auto hit = [&](int key) { bool n = glfwGetKey(w, key) == GLFW_PRESS; bool h = n && !prev[key]; prev[key] = n; return h; };
    for (int k = 1; k <= 8; k++) if (hit(GLFW_KEY_0 + k)) mode = k;
    if (hit(GLFW_KEY_B)) autoBlend = !autoBlend;
    if (hit(GLFW_KEY_R)) rotateLogo = !rotateLogo;
    if (hit(GLFW_KEY_G)) perFace = !perFace;
    if (hit(GLFW_KEY_L)) logoAsTex2 = !logoAsTex2;
}

// Вершина шейдері 11-аптадан мүлде өзгермеген
const char* vsSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vTexCoord = aTexCoord;
})";

// 2-қадам: екі sampler + blend. Атаулар C++-тағымен ДӘЛ сәйкес болуы керек (2-жиі кететін қате).
const char* fsSrc = R"(#version 330 core
in  vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;   // Қ1
uniform float blend;
uniform int   mode;
uniform float time;           // Қ4
uniform int   rotateLogo;     // Қ4
void main() {
    vec4 a = texture(texture1, vTexCoord);
    vec2 uv2 = vTexCoord;
    if (rotateLogo == 1) {    // Қ4: UV-ді фрагмент шейдерінде бұру
        vec2 c = vTexCoord - 0.5;
        float s = sin(time), co = cos(time);
        uv2 = vec2(c.x * co - c.y * s, c.x * s + c.y * co) + 0.5;
    }
    vec4 b = texture(texture2, uv2);
    if      (mode == 1) FragColor = mix(a, b, blend);               // 2-қадам / 3-тапсырма: сызықтық
    else if (mode == 2) FragColor = a * b;                          // көбейту
    else if (mode == 3) FragColor = a + b;                          // қосу
    else if (mode == 4) FragColor = clamp(a + b, 0.0, 1.0);         // қосу, шектеумен
    else if (mode == 5) FragColor = mix(a, b, b.a);                 // 4-тапсырма: коэффициент - логотиптің альфасы
    else if (mode == 6) FragColor = mix(a, b, vTexCoord.y);         // 5-тапсырма: UV-ден
    else if (mode == 7) FragColor = mix(a, b, smoothstep(0.3, 0.7, vTexCoord.y));
    else {                                                          // Қ1: үш текстура
        vec4 surface = mix(a, b, blend);
        vec4 decal = texture(texture3, uv2);
        FragColor = mix(surface, decal, decal.a);
    }
})";

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log); std::fprintf(stderr, "Shader error: %s\n", log); }
    return s;
}

// 11-аптадағы loadTexture - өзгеріссіз. (Төрт арналы PNG үшін fmt = GL_RGBA, "арна: 4" шығады.)
unsigned int loadTexture(const char* path) {
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if (data) {
        GLenum fmt = (n == 4) ? GL_RGBA : (n == 1 ? GL_RED : GL_RGB);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << path << "  " << w << "x" << h << "  арна: " << n << std::endl;
    } else {
        std::cerr << "Сурет жүктелмеді: " << path << std::endl;
    }
    stbi_image_free(data);
    return id;
}

void saveShot(const char* path, int w, int h) {
    std::vector<unsigned char> px(w * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    FILE* f = std::fopen(path, "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; y--) std::fwrite(&px[y * w * 3], 1, w * 3, f);
    std::fclose(f);
}

int main(int argc, char** argv) {
    const char* shotPath = nullptr;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (a == "--shot") shotPath = argv[++i];
        else if (a == "--blend") blend = (float)std::atof(next().c_str());
        else if (a == "--mode") mode = std::atoi(next().c_str());
        else if (a == "--tex2") logoAsTex2 = (next() == "logo");
        else if (a == "--broken-units") brokenUnits = true;
        else if (a == "--wrong-id") wrongId = true;
        else if (a == "--three") { useThree = true; mode = 8; }
        else if (a == "--per-face") perFace = true;
        else if (a == "--rotate") rotateLogo = true;
        else if (a == "--time") simTime = (float)std::atof(next().c_str());
        else if (a == "--fov") fov = (float)std::atof(next().c_str());
        else if (a == "--cam") std::sscanf(next().c_str(), "%f,%f,%f", &camPos.x, &camPos.y, &camPos.z);
    }

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 12 - Blending textures", nullptr, nullptr);
    if (!window) { std::fprintf(stderr, "Window error\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::fprintf(stderr, "GLEW error\n"); return -1; }
    glGetError();

    if (!shotPath) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetScrollCallback(window, scroll_callback);
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    GLuint vs = compile(GL_VERTEX_SHADER, vsSrc), fs = compile(GL_FRAGMENT_SHADER, fsSrc);
    GLuint shader = glCreateProgram();
    glAttachShader(shader, vs); glAttachShader(shader, fs);
    glLinkProgram(shader);
    glDeleteShader(vs); glDeleteShader(fs);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 1-қадам: екінші текстураны жүктеу (екі жол шығуы тиіс)
    std::string p1 = std::string(ASSET_DIR) + "/container.png";
    std::string p2 = std::string(ASSET_DIR) + "/pattern.png";
    std::string p3 = std::string(ASSET_DIR) + "/logo.png";
    unsigned int tex1 = loadTexture(p1.c_str());
    unsigned int tex2 = loadTexture(p2.c_str());
    unsigned int tex3 = loadTexture(p3.c_str());   // logo.png: 4-тапсырма, Қ1
    texContainer = tex1; texPattern = tex2; texLogo = tex3;
    std::cout << "tex1 = " << tex1 << "   tex2 = " << tex2 << "   tex3 = " << tex3 << std::endl;  // 2-тапсырма

    // Қ4: логотип бұрылғанда шеттері қайталанбасын - мөлдір шекара түсі
    {
        float clear[] = {0.0f, 0.0f, 0.0f, 0.0f};
        glBindTexture(GL_TEXTURE_2D, tex3);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clear);
    }
    updateFront();

    // 3-қадам: glUseProgram - АЛДЫМЕН, содан кейін бірлік нөмірлері (ID емес!)
    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
    if (wrongId) glUniform1i(glGetUniformLocation(shader, "texture2"), tex2);  // 2-тапсырма: әдейі қате
    else         glUniform1i(glGetUniformLocation(shader, "texture2"), 1);
    glUniform1i(glGetUniformLocation(shader, "texture3"), 2);

    int locModel = glGetUniformLocation(shader, "model"), locView = glGetUniformLocation(shader, "view");
    int locProj = glGetUniformLocation(shader, "projection"), locBlend = glGetUniformLocation(shader, "blend");
    int locMode = glGetUniformLocation(shader, "mode"), locTime = glGetUniformLocation(shader, "time");
    int locRot = glGetUniformLocation(shader, "rotateLogo");

    float lastTime = (float)glfwGetTime();
    int frame = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        processInput(window, dt);
        float t = simTime >= 0.0f ? simTime : now;
        if (autoBlend) blend = 0.5f + 0.5f * std::sin(t);   // Қ2

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        // 4-қадам: әр кадр байлау. glActiveTexture әрқашан glBindTexture-дан БҰРЫН.
        // 1-тапсырма: --broken-units кезінде бұл жолдар "комментарийге алынған" күйді имитациялайды.
        unsigned int second = logoAsTex2 ? tex3 : tex2;
        if (!brokenUnits) glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex1);
        if (!brokenUnits) glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, second);
        if (useThree) {   // Қ1: үшінші бірлік
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, tex3);
        }
        glActiveTexture(GL_TEXTURE0);

        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)gWidth / gHeight, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(locBlend, blend);                    // render циклінде (цикл сыртында емес!)
        glUniform1i(locMode, mode);
        glUniform1f(locTime, t);
        glUniform1i(locRot, rotateLogo ? 1 : 0);

        glBindVertexArray(vao);
        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, shotPath ? 0.4f * (i + 1) : t * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            if (perFace) {   // Қ3: алты жақ - алты blend
                for (int f = 0; f < 6; ++f) {
                    glUniform1f(locBlend, f / 5.0f);
                    glDrawArrays(GL_TRIANGLES, f * 6, 6);
                }
                glUniform1f(locBlend, blend);
            } else {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        frame++;
        if (shotPath && frame == 10) { saveShot(shotPath, gWidth, gHeight); break; }
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
