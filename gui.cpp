#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <nfd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <GLFW/glfw3.h>

#define N_RECENT_PROJECTS 5
#define MAP_LAYERS 3
#define MAX_MAP_SIZE 32

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    int tileID = -1;
    ImVec2 uv0;
    ImVec2 uv1;
} sprite;

typedef struct {
    int spriteSheetID = 0;
    int width;
    int height;
    GLuint texture;
    bool show = false;
    std::string path = "";
    sprite sprites[64];
} image;

typedef struct {
    int map[2][MAX_MAP_SIZE][MAX_MAP_SIZE][3] = {};
    int size[2] = {16, 16};
    std::string saveFile = "";
    int currentLayer = 0;
    int showLayers = 7;
} map;

map init_map (){
    map new_map = {};

    return new_map;
}

image spritesheets[10] = {};

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

// Open and read a file, then forward to LoadTextureFromMemory()
bool LoadTextureFromFile(const char* file_name, GLuint* out_texture, int* out_width, int* out_height)
{
    FILE* f = fopen(file_name, "rb");
    if (f == NULL)
        return false;
    fseek(f, 0, SEEK_END);
    size_t file_size = (size_t)ftell(f);
    if (file_size == -1)
        return false;
    fseek(f, 0, SEEK_SET);
    void* file_data = IM_ALLOC(file_size);
    fread(file_data, 1, file_size, f);
    fclose(f);
    bool ret = LoadTextureFromMemory(file_data, file_size, out_texture, out_width, out_height);
    IM_FREE(file_data);
    return ret;
}

void empty_spritesheets() {
    for (int i = 0; i < 10; i++) {
        spritesheets[i].spriteSheetID = 0;
        spritesheets[i].show = 0;
    }
}

bool load_spritesheet(std::string path) {
    int free_index = -1;
    for(int i = 0; i < 10; i++) {
        if (spritesheets[i].spriteSheetID == 0){
            free_index = i;
            break;
        } else if (spritesheets[i].path.compare(path) == 0) {
            free_index = -1;
            break;
        }
    }
    if (free_index != -1) {
        spritesheets[free_index].show = LoadTextureFromFile(path.c_str(), &spritesheets[free_index].texture, &spritesheets[free_index].width, &spritesheets[free_index].height);
        spritesheets[free_index].path = path;
        spritesheets[free_index].spriteSheetID = free_index + 1; 
    } else {
        printf("File is already open or there is no more space spritesheet slots.");
        return 0;
    }
    return 1;
}

bool saveMapState(const char* savePath, map map){
    FILE* file = std::fopen(savePath, "w");
    if (file == nullptr) return false;

    for (int i = 0; i < 10 && spritesheets[i].spriteSheetID > 0; i++){
        std::fprintf(file, "SprSh[%d]=\"%s\";\n", i, spritesheets[i].path.c_str());
    }
    for(int k = 0; k < MAP_LAYERS; k++){
        std::fputs("map=[\n", file);
        for (int i = 0; i < map.size[1]; i++) {
            std::fputs("[", file);
            for (int j = 0; j < map.size[0]; j++){
                std::fprintf(file, "(%d,%d),", map.map[0][i][j][k], map.map[1][i][j][k]);
            }
            std::fputs("]\n", file);
        }
        std::fputs("]\n", file);
    }
    std::fclose(file);

    return true;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{   
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    if (NFD_Init() != NFD_OKAY)
        return 1;

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    GLFWwindow* window = glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), "SGE", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup NFDe context
    nfdu8char_t *outPath;
    nfdu8filteritem_t imageFilters[1] = { { "Image Files", "png,jpg,jpeg" } };
    nfdu8filteritem_t mapFilters[1] = { { "Map File", "sgem" } };
    nfdopendialogu8args_t openImageArgs = {0};
    nfdopendialogu8args_t openMapArgs = {0};
    openImageArgs.filterList = imageFilters;
    openMapArgs.filterList = mapFilters;
    openImageArgs.filterCount = 1;
    openMapArgs.filterCount = 1;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
    
    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_game_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int tileWidth = 32;
    int tileHeight = 32;
    int selectedTile[2] = {-1, -1};
    map currentMap = {};
    std::string savePath = "";

    while (!glfwWindowShouldClose(window)){
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if(ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Map")) {
                    currentMap = init_map();
                    empty_spritesheets();
                    show_game_window = true;
                }
                if (ImGui::MenuItem("Open Sprite Sheet")) {
                    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &openImageArgs);
                    if (result == NFD_OKAY)
                    {
                        load_spritesheet(outPath);
                        NFD_FreePathU8(outPath);
                    }
                }
                if (ImGui::MenuItem("Open Map File")) {
                    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &openMapArgs);
                    if (result == NFD_OKAY)
                    {
                        empty_spritesheets();
                        std::ifstream file(outPath);
                        std::string line;
                        std::string filename = "";
                        int j = -1, k = 0;
                        bool readingFilename = 0;
                        bool readingMap = 0;
                        int layer = -1;

                        if (!file.is_open()){
                            printf("Error: Could not open file.\n");
                            return 1;
                        }
                        while(std::getline(file, line)) {
                            if (line[0] == ']') readingMap = 0;
                            for(int i = 0; i < line.length(); i++) {
                                if (readingMap) {
                                    if (line[i] == ',' && line[i - 1] != ')'){
                                        currentMap.map[0][j][k][layer] = (line[i - 1] - '0');
                                        i++;
                                        for (int l = 0; l < 3 && line[i + l] != ')'; l++) {
                                            filename += line[i + l];
                                        }
                                        currentMap.map[1][j][k][layer] = atoi(filename.c_str());
                                        filename = "";
                                        k++;
                                    }
                                    continue;
                                }
                                if (readingFilename) {
                                    if (line[i] == '\"') break;
                                    filename += line[i];
                                    continue;
                                }
                                if(line[i] == '=') {
                                    if (line[i - 3] != 'm'){
                                        readingFilename = 1;
                                        i++;
                                    } else {
                                        readingMap = 1;
                                        layer++;
                                        j = -1;
                                        break;
                                    }
                                }
                            }
                            if (readingFilename) {
                                load_spritesheet(filename);
                                readingFilename = 0;
                                filename = "";
                            }
                            if (readingMap) {
                                k = 0;
                                j++;
                            }
                            if (layer > MAP_LAYERS) break;
                        }
                        file.close();
                        savePath = outPath;
                    }
                    NFD_FreePathU8(outPath);
                }
                // if (ImGui::BeginMenu("Open Recent")) {
                //     if (recent_project_count == 0){
                //         ImGui::MenuItem("No recent project.", NULL, false, false);
                //     }
                //     for(int i = 0; i < recent_project_count; i++) {
                //         if (ImGui::MenuItem(recent_projects[i])) {
                //             // open_project();
                //         }
                //     }
                //     ImGui::EndMenu();
                // }
                if (ImGui::MenuItem("Save", NULL, false, savePath != "")) {
                    saveMapState(savePath.c_str(), currentMap);
                }
                if (ImGui::MenuItem("Save As...")) {
                    nfdresult_t result = NFD_SaveDialog(&outPath, mapFilters, 1, NULL, "map0.sgem");
                    if (result == NFD_OKAY)
                    {
                        saveMapState(outPath, currentMap);
                        savePath = outPath;
                    }
                    NFD_FreePathU8(outPath);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Test Windows"))
            {
                if (ImGui::MenuItem("Open Imgui Demo")) show_demo_window = true;
                ImGui::EndMenu();
            }
            ImGui::Text("%.1f FPS (average %.2f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
            ImGui::EndMainMenuBar();
        }

        int tileIndex = 1;
        int offSet = 1;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));
        for (int i = 0; i < 9; i++) {
            if (spritesheets[i].show){
                ImGui::Begin(spritesheets[i].path.c_str(), &spritesheets[i].show);
                ImGui::Text("pointer = %x", spritesheets[i].texture);
                ImGui::Text("size = %d x %d", spritesheets[i].width, spritesheets[i].height);
                ImGui::Text("selected tile = (sh%d, t%d)", selectedTile[0], selectedTile[1]);
                for (int j = 0; j < spritesheets[i].height; j+=tileHeight){
                    for (int k = 0; k < spritesheets[i].width; k+=tileWidth){
                        ImGui::PushID(tileIndex);
                        int offsetIndex = tileIndex - offSet;
                        ImVec2 uv0 = ImVec2((float)k / spritesheets[i].width, (float)j / spritesheets[i].height);
                        ImVec2 uv1 = ImVec2((float)(tileWidth + k) / spritesheets[i].width, float(tileHeight + j) / spritesheets[i].height);
                        spritesheets[i].sprites[offsetIndex].tileID = offsetIndex; 
                        spritesheets[i].sprites[offsetIndex].uv0 = uv0;
                        spritesheets[i].sprites[offsetIndex].uv1 = uv1;
                        if(ImGui::ImageButton("", (ImTextureID)(intptr_t)spritesheets[i].texture, ImVec2(tileWidth, tileHeight), uv0, uv1)){
                            selectedTile[0] = i + 1;
                            selectedTile[1] = offsetIndex;
                        }
                        ImGui::PopID();
                        ImGui::SameLine();
                        tileIndex++;
                    }
                    ImGui::NewLine();
                }
                ImGui::End();
                offSet += spritesheets[i].width >> 5 + spritesheets[i].height >> 5;
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        if (show_game_window) {
            ImGui::Begin("Testing Screen", &show_game_window);
            ImVec2 scrolling(0.0f, 0.0f);
            ImGui::Text("Layers:");
            ImGui::NewLine();
            for (int i = 0; i < MAP_LAYERS; i++){
                ImGui::PushID(i);
                ImGui::Text("Layer %d", i + 1);
                ImGui::SameLine();
                if(ImGui::Button("act")) {
                    currentMap.showLayers ^= 1 << i;
                }
                ImGui::SameLine();
                if(ImGui::Button("cur")) {
                    currentMap.currentLayer = i;
                }
                ImGui::PopID();
            }
            ImGui::SliderInt2("Map Size(w,h)", currentMap.size, 1, 32, NULL);
            ImGui::NewLine();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
            ImGui::PopStyleVar();
            ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
            ImGuiIO& io = ImGui::GetIO();
            ImVec2 canvas_p1 = ImVec2(canvas_p0.x + tileWidth * currentMap.size[0], canvas_p0.y + tileHeight * currentMap.size[1]);
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
            draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
            
            ImGui::InvisibleButton("##CanvasButton", ImVec2(tileWidth * currentMap.size[0], tileHeight * currentMap.size[1]), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            const bool is_hovered = ImGui::IsItemHovered();
            const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y);
            const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
            if (is_hovered && selectedTile[0] > 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                currentMap.map[0][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][currentMap.currentLayer] = selectedTile[0];
                currentMap.map[1][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][currentMap.currentLayer] = selectedTile[1];
            }
            if (is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                currentMap.map[0][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][currentMap.currentLayer] = 0;
            }
            
            for (int i = 0; i < currentMap.size[1]; i++){
                for (int j = 0; j < currentMap.size[0]; j++){
                    for(int k = 0; k < MAP_LAYERS; k++){
                        if (!(currentMap.showLayers & (1 << k))) continue;
                        if (currentMap.map[0][i][j][k] > 0) {
                            draw_list->AddImage(
                                (ImTextureID)(intptr_t)spritesheets[currentMap.map[0][i][j][k] - 1].texture,
                                ImVec2(origin.x + j * tileWidth, origin.y + i * tileHeight),
                                ImVec2(origin.x + (j + 1) * tileWidth, origin.y + (i + 1) * tileHeight),
                                spritesheets[currentMap.map[0][i][j][k] - 1].sprites[currentMap.map[1][i][j][k]].uv0,
                                spritesheets[currentMap.map[0][i][j][k] - 1].sprites[currentMap.map[1][i][j][k]].uv1
                            );
                        }
                    }
                    ImGui::SameLine();
                }
                ImGui::NewLine();
            }
            ImGui::EndChild();
            ImGui::End();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window); // Show demo window! :)
    
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    NFD_Quit();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}