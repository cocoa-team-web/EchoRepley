#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <vector>

using namespace geode::prelude;

// Experimental scaffold. Geode/GD SDK signatures can differ by version.
// Validate this file against the headers in your installed SDK before building.

namespace EchoReplay {
    struct Sample {
        CCPoint position;
        float elapsed;
    };

    static std::vector<Sample> currentAttempt;
    static std::vector<Sample> previousAttempt;

    static constexpr size_t MAX_SAMPLES = 12000;
    static float elapsed = 0.f;
    static float lastX = 0.f;
    static bool hasLastX = false;

    // A simple visual ghost. This is intentionally kept separate from collisions.
    static CCSprite* ghost = nullptr;
    static CCNode* ghostParent = nullptr;
    static size_t replayIndex = 0;

    static void removeGhost() {
        if (ghost) {
            ghost->removeFromParentAndCleanup(true);
        }
        ghost = nullptr;
        ghostParent = nullptr;
        replayIndex = 0;
    }

    static void startNewAttempt() {
        if (!currentAttempt.empty()) {
            previousAttempt = currentAttempt;
        }
        currentAttempt.clear();
        elapsed = 0.f;
        replayIndex = 0;
        // Keep the previous attempt available for the ghost.
    }

    static void updateGhost(PlayerObject* player) {
        if (previousAttempt.empty()) {
            removeGhost();
            return;
        }

        auto parent = player->getParent();
        if (!parent) {
            return;
        }

        if (!ghost || ghostParent != parent) {
            removeGhost();

            // This API may need adjustment for your exact GD/Geode SDK.
            ghost = CCSprite::createWithSpriteFrame(player->displayFrame());
            if (!ghost) {
                return;
            }

            ghostParent = parent;
            ghost->setOpacity(100);
            ghost->setColor({0, 220, 255});
            ghost->setID("echo-ghost"_spr);
            parent->addChild(ghost, player->getZOrder() - 1);
        }

        if (replayIndex >= previousAttempt.size()) {
            replayIndex = 0;
        }

        ghost->setPosition(previousAttempt[replayIndex].position);
        ghost->setVisible(true);

        if (replayIndex + 1 < previousAttempt.size()) {
            ++replayIndex;
        }
    }

    static void recordPlayer(PlayerObject* player, float dt) {
        const auto pos = player->getPosition();
        elapsed += dt;

        // Temporary heuristic only: an X-position jump backwards may indicate a restart.
        // It is not a reliable death/restart detector and can misfire on some gameplay.
        if (hasLastX && pos.x < lastX - 100.f) {
            startNewAttempt();
        }

        lastX = pos.x;
        hasLastX = true;

        if (currentAttempt.size() < MAX_SAMPLES) {
            currentAttempt.push_back({pos, elapsed});
        }

        updateGhost(player);
    }
}

class $modify(EchoReplayPlayerObject, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        EchoReplay::recordPlayer(this, dt);
    }
};
