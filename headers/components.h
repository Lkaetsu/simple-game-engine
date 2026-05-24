#ifndef SGE_COMPONENTS_H
#include "utils.h"

void showMainMenuBar(map *currentMap, std::string *savePath, bool *show_game_window, bool *show_demo_window) {
    ImGuiIO& io = ImGui::GetIO();
    nfdu8char_t *outPath;
    nfdu8filteritem_t imageFilters[1] = { { "Image Files", "png,jpg,jpeg" } };
    nfdu8filteritem_t mapFilters[1] = { { "Map File", "sgem" } };
    nfdopendialogu8args_t openImageArgs = {0};
    nfdopendialogu8args_t openMapArgs = {0};
    openImageArgs.filterList = imageFilters;
    openMapArgs.filterList = mapFilters;
    openImageArgs.filterCount = 1;
    openMapArgs.filterCount = 1;

    if(ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Map")) {
                *currentMap = init_map();
                emptySpritesheets(currentMap->spritesheets);
                *show_game_window = true;
            }
            if (ImGui::MenuItem("Open Sprite Sheet")) {
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &openImageArgs);
                if (result == NFD_OKAY)
                {
                    loadSpritesheet(outPath, currentMap->spritesheets);
                    NFD_FreePathU8(outPath);
                }
            }
            if (ImGui::MenuItem("Open Map File")) {
                nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &openMapArgs);
                if (result == NFD_OKAY)
                {
                    emptySpritesheets(currentMap->spritesheets);
                    *currentMap = openMap(outPath);
                    *savePath = outPath;
                    NFD_FreePathU8(outPath);
                }
            }
            if (ImGui::MenuItem("Save", NULL, false, *savePath != "")) {
                saveMapState(savePath->c_str(), *currentMap);
            }
            if (ImGui::MenuItem("Save As...")) {
                nfdresult_t result = NFD_SaveDialog(&outPath, mapFilters, 1, NULL, "map0.sgem");
                if (result == NFD_OKAY)
                {
                    saveMapState(outPath, *currentMap);
                    *savePath = outPath;
                    NFD_FreePathU8(outPath);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Test Windows"))
        {
            if (ImGui::MenuItem("Open Imgui Demo")) *show_demo_window = true;
            ImGui::EndMenu();
        }
        ImGui::Text("%.1f FPS (average %.2f ms/frame)", io.Framerate, 1000.0f / io.Framerate);
        ImGui::EndMainMenuBar();
    }
}

void showSpriteSelection(image spritesheets[MAX_SPRITESHEETS], int selectedTile[2]) {
    int tileIndex = 1;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(3.0f, 3.0f));
    for (int i = 0; i < MAX_SPRITESHEETS; i++) {
        if (spritesheets[i].show) {
            ImVec2 tileSize = ImVec2(spritesheets[i].tileWidth, spritesheets[i].tileHeight);
            int l = 0;
            ImGui::Begin(spritesheets[i].path.c_str(), &spritesheets[i].show);
            ImGui::Text("pointer = %x", spritesheets[i].texture);
            ImGui::Text("size = %d x %d", spritesheets[i].width, spritesheets[i].height);
            ImGui::Text("selected tile = (sh%d, t%d)", selectedTile[0], selectedTile[1]);
            for (int j = 0; j < spritesheets[i].height; j+=spritesheets[i].tileHeight) {
                for (int k = 0; k < spritesheets[i].width; k+=spritesheets[i].tileWidth) {
                    ImGui::PushID(tileIndex);
                    if(ImGui::ImageButton("",
                            (ImTextureID)(intptr_t)spritesheets[i].texture,
                            tileSize,
                            spritesheets[i].sprites[l].uv0,
                            spritesheets[i].sprites[l].uv1)
                    ) {
                        selectedTile[0] = i + 1;
                        selectedTile[1] = l;
                    }
                    ImGui::PopID();
                    ImGui::SameLine();
                    tileIndex++;
                    l++;
                }
                ImGui::NewLine();
            }
            ImGui::End();
        }
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

void showMapWindow(map *map, int selectedTile[2], bool* p_open) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Testing Screen", p_open);
    ImVec2 scrolling(0.0f, 0.0f);

    ImGui::Text("Layers:");
    ImGui::NewLine();
    for (int i = 0; i < MAP_LAYERS; i++){
        ImGui::PushID(i);
        ImGui::Text("Layer %d", i + 1);
        ImGui::SameLine();
        if(ImGui::Button("act")) {
            map->showLayers ^= 1 << i;
        }
        ImGui::SameLine();
        if(ImGui::Button("cur")) {
            map->currentLayer = i;
        }
        ImGui::PopID();
    }
    ImGui::SliderInt2("Map Size(w,h)", map->size, 1, 32, NULL);
    ImGui::NewLine();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoMove);
    ImGui::PopStyleVar();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + 32 * map->size[0], canvas_p0.y + 32 * map->size[1]);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));
    
    // Invisible Button to click on to draw on the map
    ImGui::InvisibleButton("##CanvasButton", ImVec2(32 * map->size[0], 32 * map->size[1]), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool is_hovered = ImGui::IsItemHovered();
    const ImVec2 origin(canvas_p0.x + scrolling.x, canvas_p0.y + scrolling.y);
    const ImVec2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
    if (is_hovered && selectedTile[0] > 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        map->map[0][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][map->currentLayer] = selectedTile[0];
        map->map[1][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][map->currentLayer] = selectedTile[1];
    }
    if (is_hovered && ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        map->map[0][(int)mouse_pos_in_canvas.y >> 5][(int)mouse_pos_in_canvas.x >> 5][map->currentLayer] = 0;
    }

    // Tile drawing on the map
    for (int i = 0; i < map->size[1]; i++){
        for (int j = 0; j < map->size[0]; j++){
            for(int k = 0; k < MAP_LAYERS; k++){
                if (!(map->showLayers & (1 << k))) continue;
                if (map->map[0][i][j][k] > 0) {
                    draw_list->AddImage(
                        (ImTextureID)(intptr_t)map->spritesheets[map->map[0][i][j][k] - 1].texture,
                        ImVec2(origin.x + j * 32, origin.y + i * 32),
                        ImVec2(origin.x + (j + 1) * 32, origin.y + (i + 1) * 32),
                        map->spritesheets[map->map[0][i][j][k] - 1].sprites[map->map[1][i][j][k]].uv0,
                        map->spritesheets[map->map[0][i][j][k] - 1].sprites[map->map[1][i][j][k]].uv1
                    );
                }
            }
            ImGui::SameLine();
        }
        ImGui::NewLine();
    }
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}

#endif