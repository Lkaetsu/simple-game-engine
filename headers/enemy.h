#ifndef SGE_ENEMY_H
#define SGE_ENEMY_H

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

#include "game_map.h"

typedef struct {
    std::vector<glm::ivec2> path;
    size_t nextStep = 0;
    glm::ivec2 lastPlayerTile = glm::ivec2(-1, -1);
} EnemyAI;


glm::vec2 tileCenter(const GameMap& map, int x, int y) {
    return glm::vec2(
        (x + 0.5f) * map.tileSize.x,
        (y + 0.5f) * map.tileSize.y
    );
}

glm::vec2 collisionCenter(const GameObject& obj) {
    glm::vec2 offset =
        obj.size * glm::vec2(
            obj.collisionArea.x,
            obj.collisionArea.y
        );

    glm::vec2 collisionSize =
        obj.size * glm::vec2(
            obj.collisionArea.z,
            obj.collisionArea.w
        );

    return obj.position + offset + collisionSize * 0.5f;
}


bool canEnemyOccupy(const GameMap& map, const GameObject& enemy, glm::vec2 center) {
    GameObject probe = enemy;

    probe.position = center;
    probe.position = collisionCenter(probe);
    probe.collisionType = RECTANGULAR;

    for (const GameObject& tile : map.tiles)
    {
        if (tile.disabled || tile.isBreakable)
            continue;

        Collision collision = checkCollision(probe, tile);

        if (std::get<0>(collision))
            return false;
    }

    return true;
}


bool canEnemyMoveBetween(
    const GameMap& map,
    const GameObject& enemy,
    glm::ivec2 from,
    glm::ivec2 to
) {
    glm::vec2 start = tileCenter(map, from.x, from.y);
    glm::vec2 end   = tileCenter(map, to.x, to.y);

    glm::vec2 delta = end - start;
    float distance = glm::length(delta);

    // Sample the corridor at intervals no larger than
    // approximately one quarter of the smaller tile dimension.
    float stepSize = std::min(
        map.tileSize.x,
        map.tileSize.y
    ) * 0.25f;

    int steps = std::max(
        1,
        static_cast<int>(std::ceil(distance / stepSize))
    );

    for (int i = 0; i <= steps; ++i)
    {
        float t = static_cast<float>(i) / steps;
        glm::vec2 center = start + delta * t;

        if (!canEnemyOccupy(map, enemy, center))
            return false;
    }

    return true;
}

std::vector<glm::ivec2> findEnemyPathBFS(
    const GameMap& map,
    const GameObject& enemy,
    glm::vec2 playerPosition
) {
    const int width = map.size[0];
    const int height = map.size[1];

    if (width <= 0 || height <= 0)
        return {};

    // Use object centers rather than top-left positions.
    glm::vec2 enemyCenter = collisionCenter(enemy);

    GameObject playerSub = enemy;
    playerSub.position = playerPosition;
    glm::vec2 playerCenter = collisionCenter(playerSub);

    auto toTile = [&](glm::vec2 pos) {
        return glm::ivec2(
            static_cast<int>(std::floor(pos.x / map.tileSize.x)),
            static_cast<int>(std::floor(pos.y / map.tileSize.y))
        );
    };

    glm::ivec2 start = toTile(enemyCenter);
    glm::ivec2 goal = toTile(playerCenter);

    auto inBounds = [&](glm::ivec2 p) {
        return p.x >= 0 && p.x < width &&
               p.y >= 0 && p.y < height;
    };

    auto walkable = [&](glm::ivec2 p) {
        return inBounds(p) &&
            canEnemyOccupy(map, enemy, tileCenter(map, p.x, p.y));
    };

    if (!inBounds(start) || !inBounds(goal))
        return {};

    if (start == goal)
        return {};

    if (!walkable(goal))
        return {};

    // The ghost may currently overlap a wall slightly
    // because of collision resolution. Allow its start tile.
    std::vector<std::vector<bool>> visited(
        height, std::vector<bool>(width, false));

    std::vector<std::vector<glm::ivec2>> parent(
        height,
        std::vector<glm::ivec2>(width, glm::ivec2(-1, -1))
    );

    std::queue<glm::ivec2> frontier;

    frontier.push(start);
    visited[start.y][start.x] = true;

    const glm::ivec2 directions[] = {
        { 0, -1 }, // up
        { 1,  0 }, // right
        { 0,  1 }, // down
        {-1,  0 }  // left
    };

    bool found = false;

    while (!frontier.empty())
    {
        glm::ivec2 current = frontier.front();
        frontier.pop();

        if (current == goal)
        {
            found = true;
            break;
        }

        for (glm::ivec2 dir : directions)
        {
            glm::ivec2 next = current + dir;

            if (!inBounds(next))
                continue;

            if (visited[next.y][next.x])
                continue;

            if (!walkable(next))
                continue;

            if (!canEnemyMoveBetween(map, enemy, current, next))
                continue;

            visited[next.y][next.x] = true;
            parent[next.y][next.x] = current;

            frontier.push(next);
        }
    }

    if (!found)
        return {};

    std::vector<glm::ivec2> path;

    for (glm::ivec2 p = goal; p != start; )
    {
        path.push_back(p);
        p = parent[p.y][p.x];
    }

    std::reverse(path.begin(), path.end());
    return path;
}

#endif